"""Packaging must reject unsafe names and preserve every existing release."""
from pathlib import Path
import importlib.util
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('zh_package', ROOT/'scripts/zh_package.py')
package = importlib.util.module_from_spec(spec)
spec.loader.exec_module(package)

class PackageSafety(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(dir=ROOT/'_toolchain',prefix='package-test-')
        self.previous = package.ROOT
        package.ROOT = Path(self.temp.name)
    def tearDown(self):
        package.ROOT = self.previous
        self.temp.cleanup()
    def test_names_stay_in_project(self):
        for value in ('../outside','C:/outside','ZH-Studio-../outside','ZH-Studio-A/B','ZH-Studio-', 'ZH-Studio-a\\b'):
            with self.subTest(value=value), self.assertRaises(ValueError): package.destination(value)
    def test_existing_release_never_overwritten(self):
        folder=package.ROOT/'ZH-Studio-Existing'
        folder.mkdir(); marker=folder/'saved-project.3mf';marker.write_bytes(b'keep this')
        with self.assertRaises(FileExistsError): package.package(folder.name)
        self.assertEqual(marker.read_bytes(),b'keep this')
        self.assertEqual(list(package.ROOT.iterdir()),[folder])
    def test_valid_new_name(self):
        folder=package.destination('ZH-Studio-Refresh-2')
        self.assertEqual(folder.parent,package.ROOT)
        self.assertFalse(folder.exists())

if __name__ == '__main__': unittest.main()
