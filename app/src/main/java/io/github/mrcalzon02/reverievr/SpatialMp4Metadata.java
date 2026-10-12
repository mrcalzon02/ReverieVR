package io.github.mrcalzon02.reverievr;

import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.nio.channels.SeekableByteChannel;
import java.nio.charset.StandardCharsets;

/**
 * Conservative, bounded ISO-BMFF Spatial Media v2 track inspection.
 * Supports standard st3d packing and unrotated, uncropped sv3d/proj/equi.
 * Does NOT treat raw video bytes, unknown mesh, or camera VR180 as a known
 * projection. Not a generic MP4 decoder or a whole-file metadata indexer.
 */
final class SpatialMp4Metadata {
    static final int MAX_SCAN_BYTES = 4 * 1024 * 1024;
    private static final int MAX_DEPTH = 12;
    private static final int MAX_TOP_LEVEL_BOXES = 1024;

    enum Status { UNKNOWN, SUPPORTED, UNSUPPORTED }

    static final class Result {
        final Status status;
        final VideoProjection projection;
        final String detail;

        private Result(Status status, VideoProjection projection, String detail) {
            this.status = status;
            this.projection = projection;
            this.detail = detail;
        }

        static Result unknown() {
            return new Result(Status.UNKNOWN, null, "No supported MP4 spatial metadata found");
        }

        static Result supported(VideoProjection projection) {
            return new Result(Status.SUPPORTED, projection, "MP4 Spatial Media v2");
        }

        static Result unsupported(String reason) {
            return new Result(Status.UNSUPPORTED, null, reason);
        }
    }

    private static final class Box {
        final int body;
        final int end;
        final String type;

        Box(int body, int end, String type) {
            this.body = body;
            this.end = end;
            this.type = type;
        }
    }

    private static final class Track {
        boolean hasStereo;
        int stereo = 0;
        boolean hasSpatial;
        boolean equirectangular;
        boolean unsupported;
        String reason = "Unsupported MP4 spatial projection";
    }

    private static final class Scan {
        Result found = Result.unknown();

        void record(Result next) {
            if (found.status == Status.UNSUPPORTED) return;
            if (next.status == Status.UNSUPPORTED) {
                found = next;
            } else if (next.status == Status.SUPPORTED) {
                if (found.status == Status.SUPPORTED
                    && found.projection != next.projection) {
                    found = Result.unsupported("Conflicting spatial video tracks");
                } else {
                    found = next;
                }
            }
        }
    }

    private SpatialMp4Metadata() { }

    static Result inspect(InputStream stream) throws IOException {
        if (stream == null) return Result.unknown();
        // Read at most 4 MiB once per selected document. A movie with moov
        // after a large mdat yields UNKNOWN rather than an unbounded scan.
        byte[] bytes = new byte[MAX_SCAN_BYTES];
        int size = 0;
        while (size < bytes.length) {
            int count = stream.read(bytes, size, bytes.length - size);
            if (count < 0) break;
            if (count == 0) {
                int one = stream.read();
                if (one < 0) break;
                bytes[size++] = (byte) one;
            } else {
                size += count;
            }
        }
        if (size < 16) return Result.unknown();
        Box first = box(bytes, 0, size);
        if (first == null
            || (!"ftyp".equals(first.type) && !"styp".equals(first.type))) {
            return Result.unknown();
        }
        Scan scan = new Scan();
        scanBoxes(bytes, first.end, size, 0, scan);
        return scan.found;
    }


    /**
     * Seekable SAF/MP4 path. Walk only top-level box headers, skipping mdat
     * by its declared size (including 64-bit extended sizes); read at most
     * MAX_SCAN_BYTES of a complete moov into memory. Thus moov at the end of
     * a many-gigabyte MP4 is reachable without decoding or scanning its
     * potentially enormous media payload. Unknown, malformed, oversized and
     * nonseekable inputs never produce invented projections.
     *
     * Caller owns the channel and is responsible for fallback to inspect
     * (InputStream) when a SAF provider only supplies a nonseekable pipe.
     */
    static Result inspect(SeekableByteChannel channel) throws IOException {
        if (channel == null) return Result.unknown();
        // Calling position first also detects pipe-backed SAF descriptors:
        // size() alone can report zero for a pipe without throwing ESPIPE.
        channel.position(0);
        long length = channel.size();
        if (length < 16) return Result.unknown();
        byte[] header = new byte[16];
        Scan scan = new Scan();
        long offset = 0;
        int boxes = 0;
        while (offset < length && boxes++ < MAX_TOP_LEVEL_BOXES) {
            if (length - offset < 8
                || !readExactlyAt(channel, offset, header, 8)) {
                return Result.unknown();
            }
            long size = unsigned32(header, 0);
            String type = new String(header, 4, 4, StandardCharsets.US_ASCII);
            if (offset == 0
                && !"ftyp".equals(type) && !"styp".equals(type)) {
                return Result.unknown();
            }

            int headerSize = 8;
            if (size == 1) {
                if (length - offset < 16
                    || !readExactlyAt(channel, offset, header, 16)) {
                    return Result.unknown();
                }
                long high = unsigned32(header, 8);
                long low = unsigned32(header, 12);
                if (high > 0x7FFFFFFFL) return Result.unknown();
                size = (high << 32) | low;
                headerSize = 16;
            } else if (size == 0) {
                size = length - offset; // extends to end of file
            }

            if (size < headerSize || size > length - offset) {
                return Result.unknown();
            }

            if ("moov".equals(type)) {
                if (size > MAX_SCAN_BYTES) return Result.unknown();
                byte[] moov = new byte[(int) size];
                if (!readExactlyAt(channel, offset, moov, moov.length)) {
                    return Result.unknown();
                }
                scanBoxes(moov, 0, moov.length, 0, scan);
            }
            offset += size;
        }
        // A cap hit before reaching the end means the inspection was not
        // exhaustive; do not silently accept an incomplete multi-track scan.
        if (offset != length) return Result.unknown();
        return scan.found;
    }

    private static boolean readExactlyAt(
            SeekableByteChannel channel, long position,
            byte[] target, int length) throws IOException {
        channel.position(position);
        ByteBuffer dest = ByteBuffer.wrap(target, 0, length);
        int emptyReads = 0;
        while (dest.hasRemaining()) {
            int read = channel.read(dest);
            if (read < 0) return false;
            if (read == 0) {
                if (++emptyReads >= 2) return false;
            } else {
                emptyReads = 0;
            }
        }
        return true;
    }

    private static void scanBoxes(byte[] b, int start, int end,
                                  int depth, Scan scan) {
        if (depth > MAX_DEPTH || start < 0 || end > b.length) return;
        for (int pos = start; pos < end;) {
            Box box = box(b, pos, end);
            if (box == null) return;  // truncated box: do not guess
            if ("moov".equals(box.type) || "trak".equals(box.type)
                || "mdia".equals(box.type) || "minf".equals(box.type)
                || "stbl".equals(box.type)) {
                scanBoxes(b, box.body, box.end, depth + 1, scan);
            } else if ("stsd".equals(box.type) && box.end - box.body >= 8) {
                // stsd is a FullBox followed by an entry_count.
                if (b[box.body] == 0) {
                    int count = (int) unsigned32(b, box.body + 4);
                    int entry = box.body + 8;
                    for (int i = 0; i < count && i < 32; i++) {
                        Box sample = box(b, entry, box.end);
                        if (sample == null) break;
                        if (isVisualSampleEntry(sample.type)) {
                            parseVisualSample(b, sample, scan);
                        }
                        entry = sample.end;
                    }
                }
            }
            pos = box.end;
        }
    }

    private static boolean isVisualSampleEntry(String type) {
        return "avc1".equals(type) || "avc3".equals(type)
            || "hvc1".equals(type) || "hev1".equals(type)
            || "mp4v".equals(type) || "av01".equals(type)
            || "vp09".equals(type) || "encv".equals(type);
    }

    private static void parseVisualSample(byte[] b, Box sample, Scan scan) {
        // ISO BMFF VisualSampleEntry has a 78-byte fixed prefix.
        int pos = sample.body + 78;
        if (pos > sample.end) return;
        Track track = new Track();
        while (pos < sample.end) {
            Box child = box(b, pos, sample.end);
            if (child == null) break;
            if ("st3d".equals(child.type)) {
                track.hasStereo = true;
                if (child.end - child.body < 5
                    || b[child.body] != 0) {
                    track.unsupported = true;
                    track.reason = "Unknown st3d version or truncated box";
                } else {
                    track.stereo = b[child.body + 4] & 0xff;
                }
            } else if ("sv3d".equals(child.type)) {
                track.hasSpatial = true;
                parseSpatial(b, child, track);
            }
            pos = child.end;
        }
        if (!track.hasSpatial && !track.hasStereo) return;
        if (track.unsupported || track.stereo > 2) {
            scan.record(Result.unsupported(track.unsupported
                ? track.reason : "Unsupported st3d stereo layout"));
            return;
        }
        if (track.hasSpatial && !track.equirectangular) {
            scan.record(Result.unsupported(track.reason));
            return;
        }
        VideoProjection choice;
        if (track.hasSpatial) {
            choice = track.stereo == 1
                ? VideoProjection.STEREO_EQUIRECTANGULAR_360_TB
                : track.stereo == 2
                    ? VideoProjection.STEREO_EQUIRECTANGULAR_360_SBS
                    : VideoProjection.MONO_EQUIRECTANGULAR_360;
        } else {
            choice = track.stereo == 1 ? VideoProjection.FLAT_STEREO_TB
                : track.stereo == 2 ? VideoProjection.FLAT_STEREO_SBS
                : VideoProjection.FLAT_CINEMA;
        }
        scan.record(Result.supported(choice));
    }

    private static void parseSpatial(byte[] b, Box spatial, Track track) {
        int pos = spatial.body;
        boolean sawProjection = false;
        while (pos < spatial.end) {
            Box child = box(b, pos, spatial.end);
            if (child == null) {
                track.unsupported = true;
                track.reason = "Malformed sv3d container";
                return;
            }
            if ("proj".equals(child.type)) {
                sawProjection = true;
                parseProjection(b, child, track);
            }
            pos = child.end;
        }
        if (!sawProjection) {
            track.unsupported = true;
            track.reason = "Spatial video lacks a projection box";
        }
    }

    private static void parseProjection(byte[] b, Box proj, Track track) {
        int pos = proj.body;
        boolean sawEquirect = false;
        while (pos < proj.end) {
            Box child = box(b, pos, proj.end);
            if (child == null) {
                track.unsupported = true;
                track.reason = "Malformed spatial projection";
                return;
            }
            if ("equi".equals(child.type)) {
                if (sawEquirect || child.end - child.body < 20
                    || b[child.body] != 0) {
                    track.unsupported = true;
                    track.reason = "Invalid equirectangular projection";
                } else {
                    sawEquirect = true;
                    // Equi FullBox v0 followed by four crop bounds. Nonzero
                    // values require texture crop math not implemented here.
                    for (int offset = 4; offset < 20; offset += 4) {
                        if (unsigned32(b, child.body + offset) != 0) {
                            track.unsupported = true;
                            track.reason = "Cropped equirectangular projection";
                        }
                    }
                }
            } else if ("prhd".equals(child.type)) {
                if (child.end - child.body < 16
                    || b[child.body] != 0) {
                    track.unsupported = true;
                    track.reason = "Invalid projection pose";
                } else {
                    // Ignore nonzero rotations until the renderer applies
                    // the 16.16 pose values correctly.
                    for (int offset = 4; offset < 16; offset += 4) {
                        if (unsigned32(b, child.body + offset) != 0) {
                            track.unsupported = true;
                            track.reason = "Spatial pose correction required";
                        }
                    }
                }
            } else if ("mshp".equals(child.type)
                    || "cbmp".equals(child.type)) {
                track.unsupported = true;
                track.reason = "Calibrated mesh or cubemap projection unsupported";
            }
            pos = child.end;
        }
        track.equirectangular = sawEquirect && !track.unsupported;
    }

    private static Box box(byte[] b, int pos, int limit) {
        if (pos < 0 || limit - pos < 8) return null;
        long length = unsigned32(b, pos);
        int header = 8;
        if (length == 1) {
            if (limit - pos < 16) return null;
            long high = unsigned32(b, pos + 8);
            long low = unsigned32(b, pos + 12);
            if (high != 0) return null;
            length = low;
            header = 16;
        } else if (length == 0) {
            length = limit - pos;
        }
        if (length < header || length > limit - pos) return null;
        String type = new String(b, pos + 4, 4,
            java.nio.charset.StandardCharsets.US_ASCII);
        return new Box(pos + header, (int) (pos + length), type);
    }

    private static long unsigned32(byte[] b, int pos) {
        return ((long) (b[pos] & 255) << 24)
            | ((long) (b[pos + 1] & 255) << 16)
            | ((long) (b[pos + 2] & 255) << 8)
            | (b[pos + 3] & 255);
    }
}
