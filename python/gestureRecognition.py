"""Finger counting and debouncing for the elevator gesture feature."""

import math

WRIST = 0
MIDDLE_MCP = 9

# (PIP, TIP) per finger: index, middle, ring, pinky. The thumb is skipped
# because it folds sideways instead of curling toward the wrist.
FINGER_JOINTS = ((6, 8), (10, 12), (14, 16), (18, 20))

EXTENSION_RATIO = 1.15   # tip must be this much further from the wrist than the PIP
MIN_PALM_PIXELS = 45     # smaller than this and the hand is too far away

# 1 = index, 2 = index+middle, 3 = index+middle+ring. Anything else is rejected.
VALID_PATTERNS = {
    (True, False, False, False): 1,
    (True, True, False, False): 2,
    (True, True, True, False): 3,
}

STABLE_FRAMES = 15       # about 0.66s at 22.8 fps
RELEASE_FRAMES = 10
COOLDOWN_SEC = 3.0
MISS_TOLERANCE = 3       # dropped frames tolerated mid-hold
DRIFT_TOLERANCE = 1.5    # multiples of palm size


def _distance(a, b):
  return math.hypot(a[0] - b[0], a[1] - b[1])


def to_pixels(hand_landmarks, frame_width, frame_height):
  """Converts normalized landmarks to (x, y) pixel tuples.

  Normalized units are not square (at 1280x960 an x-unit is 1.33x a y-unit),
  so all distances below are measured in pixels.
  """
  return [(lm.x * frame_width, lm.y * frame_height) for lm in hand_landmarks]


def palm_size(points):
  """Wrist to middle-finger MCP, used as the hand's scale reference."""
  return _distance(points[WRIST], points[MIDDLE_MCP])


def is_extended(points, pip, tip):
  """True when a finger is straight rather than curled.

  Measuring both distances from the wrist makes this immune to hand
  rotation, unlike the usual tip.y < pip.y test.
  """
  wrist = points[WRIST]
  return (_distance(wrist, points[tip])
          > EXTENSION_RATIO * _distance(wrist, points[pip]))


def classify(hand_landmarks_list, frame_width, frame_height):
  """Returns (count, anchor, scale). count is 1-3, or None if unusable."""
  # Two hands is ambiguous, so treat it as no gesture.
  if len(hand_landmarks_list) != 1:
    return None, None, 0.0

  points = to_pixels(hand_landmarks_list[0], frame_width, frame_height)
  scale = palm_size(points)
  if scale < MIN_PALM_PIXELS:
    return None, None, scale

  flags = tuple(is_extended(points, pip, tip) for pip, tip in FINGER_JOINTS)
  return VALID_PATTERNS.get(flags), points[WRIST], scale


class GestureLatch:
  """Debounces a per-frame count down to one request per gesture.

  Same idea as the STM32 pushbutton flag: hold steady to fire once, then the
  hand must be withdrawn before it can fire again. `now` is passed in so a
  test can drive a scripted timeline.
  """

  def __init__(self):
    self.state = 'ARMED'
    self.candidate = None
    self.stable = 0
    self.misses = 0
    self.released = 0
    self.saw_release = False
    self.last_fire = -COOLDOWN_SEC
    self.anchor = None

  def status(self):
    return self.state, self.candidate, self.stable

  def reset(self):
    self.__init__()

  def update(self, count, anchor, scale, now):
    """Feeds one frame in. Returns a floor to request, or None."""
    if self.state == 'LATCHED':
      # The hand must leave before another request is allowed. Remember that
      # it happened, so releasing during the cooldown still counts.
      if count is None:
        self.released += 1
        if self.released >= RELEASE_FRAMES:
          self.saw_release = True
      else:
        self.released = 0
      if self.saw_release and now - self.last_fire >= COOLDOWN_SEC:
        self.state = 'ARMED'
        self.candidate = None
        self.stable = 0
      return None

    if count is None:
      # Tolerate a few dropped frames before giving up on the hold.
      self.misses += 1
      if self.misses > MISS_TOLERANCE:
        self.candidate = None
        self.stable = 0
      return None

    self.misses = 0

    # A hand sweeping past can hold a valid pattern long enough to fire, so
    # the wrist has to stay put too.
    drifted = (self.anchor is not None and anchor is not None and scale > 0
               and _distance(anchor, self.anchor) > DRIFT_TOLERANCE * scale)

    if count != self.candidate or drifted:
      self.candidate = count
      self.stable = 1
      self.anchor = anchor
      return None

    self.stable += 1
    if self.stable >= STABLE_FRAMES:
      self.state = 'LATCHED'
      self.released = 0
      self.saw_release = False
      self.last_fire = now
      return count
    return None
