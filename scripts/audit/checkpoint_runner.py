#!/usr/bin/env python3
"""Run independent diagnostic cases with durable, resumable evidence.

This does not publish, install, alter release gates, or label skipped work passed.
Commands are argument arrays, never shell strings. Use a fresh output directory
when source/commands change; --resume reuses only matching completed evidence.
"""
import argparse
import datetime
import fcntl
import hashlib
import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import time


def utc():
    return datetime.datetime.now(datetime.timezone.utc).isoformat()


def atomic_json(path, value):
    temporary = path.with_suffix(path.suffix + '.partial')
    with temporary.open('w', encoding='utf-8') as output:
        json.dump(value, output, indent=2, sort_keys=True)
        output.write('\n')
        output.flush()
        os.fsync(output.fileno())
    os.replace(temporary, path)
    directory = os.open(path.parent, os.O_RDONLY)
    try:
        os.fsync(directory)
    finally:
        os.close(directory)


def source_identity(root):
    head = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=root, text=True).strip()
    files = subprocess.check_output(['git', 'ls-files', '-z'], cwd=root).decode().split('\0')
    digest = hashlib.sha256()
    for name in sorted(files):
        if not name or name.startswith(('docs/', 'art/', '.github/')):
            continue
        path = root / name
        digest.update(name.encode() + b'\0')
        digest.update(path.read_bytes() if path.is_file() else b'<missing>')
    return {'head_at_start': head, 'tracked_source_sha256': digest.hexdigest()}


def group_exists(group):
    try:
        os.killpg(group, 0)
        return True
    except ProcessLookupError:
        return False


def stop_process(process):
    # A leader may exit while one of its children keeps the group alive.
    try:
        os.killpg(process.pid, signal.SIGTERM)
    except ProcessLookupError:
        pass
    try:
        process.wait(timeout=5)
    except subprocess.TimeoutExpired:
        pass
    if group_exists(process.pid):
        try:
            os.killpg(process.pid, signal.SIGKILL)
        except ProcessLookupError:
            pass
    if process.poll() is None:
        process.wait(timeout=5)


def run(args):
    root = args.root.resolve()
    cases = json.loads(args.manifest.read_text(encoding='utf-8'))['cases']
    ids = [case['id'] for case in cases]
    if len(ids) != len(set(ids)):
        raise ValueError('Duplicate diagnostic case IDs')
    if args.only and set(args.only) - set(ids):
        raise ValueError('Unknown case selected')
    for case in cases:
        if not case['id'].replace('-', '').replace('_', '').isalnum():
            raise ValueError('Invalid diagnostic case ID')
        if not isinstance(case['command'], list) or not case['command']:
            raise ValueError('Commands must be nonempty argument arrays')
        if not all(isinstance(arg, str) for arg in case['command']):
            raise ValueError('Command arguments must be strings')
    args.output.mkdir(parents=True, exist_ok=True)
    with (args.output / '.lock').open('w') as lock:
        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        return locked_run(root, cases, args, lock.fileno())


def locked_run(root, cases, args, lock_fd):
    checkpoint = args.output / 'checkpoint.json'
    identity = source_identity(root)
    manifest_hash = hashlib.sha256(json.dumps(cases, sort_keys=True).encode()).hexdigest()
    if checkpoint.exists():
        if not args.resume:
            raise ValueError('Existing evidence: use --resume or a new output directory')
        state = json.loads(checkpoint.read_text(encoding='utf-8'))
        if (state['source']['tracked_source_sha256'] != identity['tracked_source_sha256']
                or state['manifest_sha256'] != manifest_hash):
            raise ValueError('Source or commands changed; preserve this evidence and use a new output directory')
    else:
        state = {'schema': 1, 'source': identity, 'manifest_sha256': manifest_hash,
                 'started_at': utc(), 'cases': {}}
    boot_path = Path('/proc/sys/kernel/random/boot_id')
    boot_id = boot_path.read_text().strip() if boot_path.is_file() else None
    # Verification must precede --only filtering: final success covers all cases.
    for case_id, entry in state['cases'].items():
        for prior in entry['attempts']:
            if prior.get('log_sha256'):
                old_log = args.output / prior['log']
                if (not old_log.is_file()
                        or hashlib.sha256(old_log.read_bytes()).hexdigest() != prior['log_sha256']):
                    raise ValueError('Completed evidence missing or changed: ' + case_id)
            if prior['status'] == 'running':
                if not args.recover_interrupted:
                    raise ValueError('Unclean termination for ' + case_id
                                     + '; inspect and stop its process group before --recover-interrupted')
                group = prior.get('process_group')
                if group and prior.get('boot_id') == boot_id and group_exists(group):
                    raise ValueError('Prior process group still exists for ' + case_id)
                # If identity was not recorded before the kill, this flag is an
                # explicit acknowledgement that external orphan cleanup was done.
                prior['status'] = 'interrupted'
                prior['recovered_at'] = utc()
                old_log = args.output / prior['log']
                if old_log.is_file():
                    prior['log_sha256'] = hashlib.sha256(old_log.read_bytes()).hexdigest()
    state['last_resumed_at'] = utc()
    atomic_json(checkpoint, state)
    try:
        for case in cases:
            if args.only and case['id'] not in args.only:
                continue
            entry = state['cases'].setdefault(case['id'], {'attempts': []})
            prior = entry['attempts'][-1] if entry['attempts'] else None
            if prior and prior['status'] == 'passed':
                print('REUSED ' + case['id'], flush=True)
                continue
            if prior and prior['status'] in ('failed', 'timed_out') and not args.retry_failed:
                print('RETAINED ' + prior['status'] + ' ' + case['id'], flush=True)
                continue
            number = len(entry['attempts']) + 1
            log_name = case['id'] + '-attempt-' + str(number) + '.log'
            attempt = {'command': case['command'], 'layer': case.get('layer', 'unspecified'),
                       'started_at': utc(), 'status': 'running', 'log': log_name,
                       'timeout_seconds': case.get('timeout_seconds', 180)}
            entry['attempts'].append(attempt)
            atomic_json(checkpoint, state)
            started = time.monotonic()
            process = None
            print('RUN ' + case['id'], flush=True)
            with (args.output / log_name).open('wb', buffering=0) as output:
                output.write((json.dumps(attempt) + '\n').encode())
                try:
                    process = subprocess.Popen(case['command'], cwd=root, stdout=output,
                                               stderr=subprocess.STDOUT, start_new_session=True,
                                               pass_fds=(lock_fd,))
                    attempt.update(process_group=process.pid, boot_id=boot_id)
                    atomic_json(checkpoint, state)
                    result = process.wait(timeout=attempt['timeout_seconds'])
                    attempt['exit_code'] = result
                    attempt['status'] = 'passed' if result == 0 else 'failed'
                except subprocess.TimeoutExpired:
                    stop_process(process)
                    attempt['status'] = 'timed_out'
                    attempt['exit_code'] = process.returncode
                except OSError as error:
                    attempt['status'] = 'failed'
                    attempt['error'] = str(error)
                    output.write((str(error) + '\n').encode())
                except (KeyboardInterrupt, SystemExit):
                    if process is not None:
                        stop_process(process)
                    attempt['status'] = 'interrupted'
                    raise
                finally:
                    attempt['duration_seconds'] = round(time.monotonic() - started, 3)
                    attempt['finished_at'] = utc()
                    os.fsync(output.fileno())
                    attempt['log_sha256'] = hashlib.sha256((args.output / log_name).read_bytes()).hexdigest()
                    atomic_json(checkpoint, state)
            print(attempt['status'].upper() + ' ' + case['id'], flush=True)
    finally:
        state['updated_at'] = utc()
        state['summary'] = {key: value['attempts'][-1]['status'] for key, value in state['cases'].items() if value['attempts']}
        atomic_json(checkpoint, state)
    return 0 if len(state['summary']) == len(cases) and all(value == 'passed' for value in state['summary'].values()) else 1


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument('--manifest', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--resume', action='store_true')
    parser.add_argument('--retry-failed', action='store_true')
    parser.add_argument('--recover-interrupted', action='store_true',
                        help='Acknowledge external cleanup after an unclean kill; refuses known live process groups')
    parser.add_argument('--only', action='append', help='Run/retry only this case; may be repeated')
    options = parser.parse_args()
    signal.signal(signal.SIGTERM, lambda *_: sys.exit(143))
    try:
        sys.exit(run(options))
    except (ValueError, BlockingIOError) as error:
        print(str(error), file=sys.stderr)
        sys.exit(2)
