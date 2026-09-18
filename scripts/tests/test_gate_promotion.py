from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).parents[1]))
import promote_coherence as promotion


class PromotionTests(unittest.TestCase):
    def test_preserves_existing_contexts_and_strict_policy(self):
        runs = [{"name":name,"status":"completed","conclusion":"success","app":{"slug":"github-actions","id":15368}}
                for name in promotion.STATUS_NAMES]
        payload = promotion.required_payload({"strict":False,"contexts":["existing CI"],"checks":[]},runs)
        self.assertFalse(payload["strict"])
        self.assertIn({"context":"existing CI","app_id":-1},payload["checks"])
        self.assertEqual(len(payload["checks"]),3)
        self.assertEqual(promotion.required_payload(payload,runs),payload)

    def test_missing_or_failed_status_cannot_be_required_as_verified(self):
        for conclusion in ("failure","skipped","cancelled",None):
            runs = [{"name":name,"status":"completed","conclusion":conclusion,"app":{"slug":"github-actions","id":15368}}
                    for name in promotion.STATUS_NAMES]
            with self.assertRaises(ValueError): promotion.required_payload({},runs)


if __name__ == "__main__": unittest.main()
