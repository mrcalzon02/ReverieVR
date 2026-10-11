import pathlib
import struct
import tempfile
import unittest
import numpy as np
from convert_stl_parts import allocate, convert, load_stl, runtime_vertices, weld

TETRA = np.asarray([[[0,0,0],[0,1,0],[1,0,0]], [[0,0,0],[1,0,0],[0,0,1]],
                    [[0,0,0],[0,0,1],[0,1,0]], [[1,0,0],[0,1,0],[0,0,1]]], dtype='<f4')

def write_stl(path, triangles=TETRA):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open('wb') as f:
        f.write(b'solid binary'.ljust(80, b'\0'))
        f.write(struct.pack('<I', len(triangles)))
        for face in triangles:
            f.write(struct.pack('<12fH', *([0.,0.,0.] + face.flatten().tolist()), 0))

class PipelineTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(dir=pathlib.Path(__file__).parent)
        self.root = pathlib.Path(self.tmp.name)
    def tearDown(self):
        self.tmp.cleanup()
    def test_binary_solid_header_and_nonfinite_rejection(self):
        path = self.root / 'valid.stl'
        write_stl(path)
        np.testing.assert_array_equal(load_stl(path), TETRA)
        bad = TETRA.copy()
        bad[0,0,0] = np.nan
        write_stl(path, bad)
        with self.assertRaises(ValueError):
            load_stl(path)
    def test_budget_is_global_and_preserves_small_parts(self):
        budget = allocate([100000, 4, 4000, 2000], 1000)
        self.assertEqual(sum(budget), 1000)
        self.assertEqual(budget[1], 4)
        with self.assertRaises(ValueError):
            allocate([1000]*22, 100)
    def test_callable_conversion_and_collision_free_names(self):
        src, out = self.root/'source', self.root/'output'
        write_stl(src/'left'/'button.stl')
        write_stl(src/'right'/'button.stl')
        report = convert(src, out, 100)
        self.assertEqual(report['total_faces'], 8)
        self.assertEqual(len({p['name'] for p in report['parts']}), 2)
        self.assertEqual((out/'controller.rvmesh').read_bytes()[:8], b'RVSC0001')
        self.assertTrue(all(p['topology']['boundary_edges'] == 0 for p in report['parts']))
        with self.assertRaises(ValueError):
            convert(src, out, 100)
        with self.assertRaises(ValueError):
            convert(src, src/'output', 100)
    def test_runtime_preserves_orientation_and_units(self):
        v, f = weld(TETRA)
        packed = runtime_vertices(v, f)
        self.assertEqual(packed.shape, (12, 6))
        np.testing.assert_allclose(np.linalg.norm(packed[:,3:], axis=1), 1, atol=1e-6)
        self.assertAlmostEqual(float(packed[:,1].max()), .001)
        self.assertAlmostEqual(float(packed[:,2].min()), -.001)
    def test_truncated_binary_rejected(self):
        path = self.root/'bad.stl'
        write_stl(path)
        path.write_bytes(path.read_bytes()[:-5])
        with self.assertRaises((ValueError, UnicodeDecodeError)):
            load_stl(path)

if __name__ == '__main__':
    unittest.main()
