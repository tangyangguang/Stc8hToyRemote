import importlib.util
import pathlib
import tempfile
import textwrap
import unittest


ROOT_DIR = pathlib.Path(__file__).resolve().parents[1]
CHECKER_PATH = ROOT_DIR / "tools" / "check_firmware_size.py"


spec = importlib.util.spec_from_file_location("check_firmware_size", CHECKER_PATH)
check_firmware_size = importlib.util.module_from_spec(spec)
spec.loader.exec_module(check_firmware_size)


class FirmwareMemCheckTest(unittest.TestCase):
    def write_mem(self, text):
        tmp = tempfile.NamedTemporaryFile("w", encoding="utf-8", delete=False)
        with tmp:
            tmp.write(textwrap.dedent(text))
        mem_path = pathlib.Path(tmp.name)
        self.addCleanup(mem_path.unlink)
        return mem_path

    def test_parses_flash_stack_and_spare_internal_ram(self):
        mem_path = self.write_mem(
            """
            Internal RAM layout:
            Stack starts at: 0x6f (sp set to 0x6e) with 145 bytes available.
            The largest spare internal RAM space starts at 0x1e with 2 bytes available.

            Other memory:
               ROM/EPROM/FLASH  0x0000   0x1af5    6902     8192
            """
        )

        usage = check_firmware_size.parse_mem_usage(mem_path)

        self.assertEqual(usage.flash_used, 6902)
        self.assertEqual(usage.stack_available, 145)
        self.assertEqual(usage.largest_spare_internal_ram, 2)

    def test_treats_no_spare_internal_ram_as_zero(self):
        mem_path = self.write_mem(
            """
            Internal RAM layout:
            Stack starts at: 0x79 (sp set to 0x78) with 135 bytes available.
            No spare internal RAM space left.

            Other memory:
               ROM/EPROM/FLASH  0x0000   0x1e6b    7788     8192
            """
        )

        usage = check_firmware_size.parse_mem_usage(mem_path)

        self.assertEqual(usage.stack_available, 135)
        self.assertEqual(usage.largest_spare_internal_ram, 0)

    def test_rejects_stack_or_spare_regressions(self):
        usage = check_firmware_size.MemUsage(
            flash_used=6902,
            stack_available=144,
            largest_spare_internal_ram=1,
        )

        errors = check_firmware_size.check_limits(
            name="receiver",
            usage=usage,
            flash_limit=8192,
            flash_target=6904,
            min_stack_available=145,
            min_largest_spare_internal_ram=2,
        )

        self.assertIn("receiver stack available 144 < 145", errors)
        self.assertIn("receiver largest spare internal RAM 1 < 2", errors)


if __name__ == "__main__":
    unittest.main()
