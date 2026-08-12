import os
import sys

# Make koan_utils importable from every koan's test module.
_KOANS = os.path.join(os.path.dirname(os.path.abspath(__file__)), "koans")
if _KOANS not in sys.path:
    sys.path.insert(0, _KOANS)
