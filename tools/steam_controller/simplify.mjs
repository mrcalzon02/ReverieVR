import {MeshoptSimplifier} from 'meshoptimizer';
import fs from 'node:fs';
import path from 'node:path';
await MeshoptSimplifier.ready;
const folder = process.argv[2], budget = Number(process.argv[3]);
const p = fs.readFileSync(path.join(folder, 'points'));
const f = fs.readFileSync(path.join(folder, 'faces'));
const points = new Float32Array(p.buffer, p.byteOffset, p.length / 4);
const faces = new Uint32Array(f.buffer, f.byteOffset, f.length / 4);
// No Sloppy/Prune: preserve named parts and connected topology. The final
// budget, topology and silhouette must still be independently inspected.
const [out, error] = MeshoptSimplifier.simplify(faces, points, 3, budget * 3, 0.02);
fs.writeFileSync(path.join(folder, 'out'), Buffer.from(out.buffer, out.byteOffset, out.byteLength));
console.log(error);
