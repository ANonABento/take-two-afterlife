"""Engine-free check for maps an artist dressed by hand.

A map is hand-dressed when any level actor, or its World Settings actor, carries the
`TTA_HandDressed` tag. Layout/dress scripts must not touch such a map.
"""

HAND_DRESSED_TAG = "TTA_HandDressed"


def _tags(actor):
    if actor is None:
        return []
    try:
        return [str(t) for t in actor.get_editor_property("tags")]
    except Exception:  # actors without a tags property (or a stale reference) carry no marker
        return []


def is_hand_dressed(actors, world_settings=None):
    """True when the marker tag is on World Settings or on any of `actors`."""
    if HAND_DRESSED_TAG in _tags(world_settings):
        return True
    return any(HAND_DRESSED_TAG in _tags(a) for a in actors)
