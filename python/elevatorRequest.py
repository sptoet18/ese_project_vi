"""Posts gesture floor requests into can_transaction from a worker thread.

The insert blocks on MariaDB and the capture loop only has ~44ms per frame,
so it is never done inline.
"""

import queue
import threading
import time

import pymysql

# 768 is the website Car Controller node. Reusing it means the supervisor
# decodes gesture requests with no firmware change; the message column is
# what tells them apart.
GESTURE_CAN_ID = 768
SAME_FLOOR_REPEAT_SEC = 8.0

DB = dict(host='127.0.0.1', user='Emiliano', password='ESE',
          database='elevator', charset='utf8mb4', autocommit=True,
          connect_timeout=3, read_timeout=3, write_timeout=3)


class ElevatorRequester:
  """Queue in front of MariaDB so the video loop never blocks."""

  def __init__(self, dry_run=False):
    self.dry_run = dry_run
    self.queue = queue.Queue(maxsize=8)
    self.last = {}
    self.note = 'idle'
    self.thread = None

  def start(self):
    self.thread = threading.Thread(target=self._worker, daemon=True)
    self.thread.start()

  def request(self, floor):
    """Non-blocking. Returns False when suppressed or dropped."""
    now = time.time()
    if now - self.last.get(floor, -SAME_FLOOR_REPEAT_SEC) < SAME_FLOOR_REPEAT_SEC:
      self.note = 'floor %d repeated' % floor
      return False
    self.last[floor] = now
    try:
      self.queue.put_nowait(floor)
      self.note = 'queued floor %d' % floor
      return True
    except queue.Full:
      self.note = 'queue full'
      return False

  def stop(self):
    self.queue.put(None)
    if self.thread:
      self.thread.join(timeout=2.0)

  def _worker(self):
    conn = None
    while True:
      floor = self.queue.get()
      if floor is None:
        break
      if self.dry_run:
        self.note = 'dry run floor %d' % floor
        continue
      try:
        if conn is None:
          conn = pymysql.connect(**DB)
        with conn.cursor() as cur:
          # Same columns request-floor.php fills in.
          cur.execute('SELECT current_floor, last_floor FROM elevator_position'
                      ' ORDER BY recorded_at DESC, id DESC LIMIT 1')
          row = cur.fetchone()
          current, last = (row[0], row[1]) if row else (1, 1)
          cur.execute(
              'INSERT INTO can_transaction'
              ' (sent_by, transceived_at, data, message,'
              '  current_floor, last_floor)'
              ' VALUES (%s, NOW(), %s, %s, %s, %s)',
              (GESTURE_CAN_ID, floor,
               'Gesture control requested floor %d' % floor, current, last))
        self.note = 'sent floor %d' % floor
      except pymysql.MySQLError as exc:
        # A database problem must never reach the video loop.
        if conn is not None:
          try:
            conn.close()
          except pymysql.MySQLError:
            pass
        conn = None
        self.note = 'db error: %s' % exc
        time.sleep(0.5)
