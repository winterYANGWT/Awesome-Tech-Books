import unittest

from worker import extract_title


class ExtractTitleTest(unittest.TestCase):
    def test_basic_title(self):
        self.assertEqual(extract_title("<html><head><title>Shorty</title></head></html>"), "Shorty")

    def test_attributes_entities_and_whitespace(self):
        html = '<TITLE data-x="1"> Hello &amp;\n  world </TITLE>'
        self.assertEqual(extract_title(html), "Hello & world")

    def test_missing_title(self):
        self.assertIsNone(extract_title("<html><body><h1>没有标题</h1></body></html>"))

    def test_empty_title(self):
        self.assertIsNone(extract_title("<html><head><title>   </title></head></html>"))

    def test_unclosed_title(self):
        self.assertEqual(extract_title("<html><head><title>未闭合"), "未闭合")

    def test_title_is_truncated(self):
        title = extract_title("<title>" + "x" * 1000 + "</title>")
        self.assertEqual(len(title), 512)


if __name__ == "__main__":
    unittest.main()
