# Copyright 2023 The MediaPipe Authors. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# ---------------------------------------------------------------------------
# Patched for the ESE Project VI elevator project, running on a
# Raspberry Pi Camera Module 3:
#
#  1) mediapipe 1.0.0 removed the legacy `mediapipe.solutions` /
#     `mediapipe.framework` modules the original example used for drawing.
#     Detection itself (mediapipe.tasks.python.vision.HandLandmarker) is
#     untouched. Landmarks are now drawn manually with OpenCV, using the
#     still-supported HandLandmarksConnections class for the skeleton.
#
#  2) Camera Module 3 is libcamera-only and is not reachable through
#     cv2.VideoCapture(). Frames are now captured with Picamera2 instead,
#     then handed to OpenCV/mediapipe exactly as before.
# ---------------------------------------------------------------------------
"""Main script to run hand landmarker on a Raspberry Pi Camera Module 3."""

import argparse
import sys
import time

import cv2
import mediapipe as mp
from mediapipe.tasks import python
from mediapipe.tasks.python import vision
from mediapipe.tasks.python.vision.hand_landmarker import HandLandmarksConnections
from picamera2 import Picamera2

import gestureRecognition
import elevatorRequest

# Global variables to calculate FPS
COUNTER, FPS = 0, 0
START_TIME = time.time()
DETECTION_RESULT = None


def run(model: str, num_hands: int,
        min_hand_detection_confidence: float,
        min_hand_presence_confidence: float, min_tracking_confidence: float,
        camera_id: int, width: int, height: int) -> None:
  """Continuously run inference on images acquired from the Pi camera.

  Args:
    model: Name of the hand landmarker model bundle.
    num_hands: Max number of hands that can be detected by the landmarker.
    min_hand_detection_confidence: The minimum confidence score for hand
      detection to be considered successful.
    min_hand_presence_confidence: The minimum confidence score of hand
      presence score in the hand landmark detection.
    min_tracking_confidence: The minimum confidence score for the hand
      tracking to be considered successful.
    camera_id: Which physical camera to use (Picamera2 camera_num). 0 for a
      single Camera Module.
    width: The width of the frame captured from the camera.
    height: The height of the frame captured from the camera.
  """

  # Start capturing video input from the Raspberry Pi Camera Module 3.
  # Requesting 'BGR888' gives frames in the same byte order OpenCV expects,
  # so the rest of the pipeline below is unchanged from the cv2.VideoCapture
  # version.
  picam2 = Picamera2(camera_num=camera_id)
  config = picam2.create_preview_configuration(
      main={'size': (width, height), 'format':'RGB888'})
  picam2.configure(config)
  picam2.start()
  time.sleep(1)  # brief warm-up so auto-exposure/auto-focus can settle

  # Visualization parameters
  row_size = 50  # pixels
  left_margin = 24  # pixels
  text_color = (0, 0, 0)  # black
  font_size = 1
  font_thickness = 1
  fps_avg_frame_count = 10
  handedness_text_color = (88, 205, 54)  # vibrant green

  def save_result(result: vision.HandLandmarkerResult,
                   unused_output_image: mp.Image, timestamp_ms: int):
    global FPS, COUNTER, START_TIME, DETECTION_RESULT
    # Calculate the FPS
    if COUNTER % fps_avg_frame_count == 0:
      FPS = fps_avg_frame_count / (time.time() - START_TIME)
      START_TIME = time.time()

    DETECTION_RESULT = result
    COUNTER += 1

  # Initialize the hand landmarker model
  base_options = python.BaseOptions(model_asset_path=model)
  options = vision.HandLandmarkerOptions(
      base_options=base_options,
      running_mode=vision.RunningMode.LIVE_STREAM,
      num_hands=num_hands,
      min_hand_detection_confidence=min_hand_detection_confidence,
      min_hand_presence_confidence=min_hand_presence_confidence,
      min_tracking_confidence=min_tracking_confidence,
      result_callback=save_result)
  detector = vision.HandLandmarker.create_from_options(options)

  # Gesture -> floor request. The requester writes on its own thread so this
  # loop never waits on the database.
  latch = gestureRecognition.GestureLatch()
  requester = elevatorRequest.ElevatorRequester(dry_run=False)
  requester.start()

  # Continuously capture images from the camera and run inference
  while True:
    image = picam2.capture_array()
    if image is None:
      sys.exit(
          'ERROR: Unable to read from Picamera2. Please verify the camera '
          'connection and that no other process is using it.')

    image = cv2.flip(image, 1)

    # Convert the image from BGR to RGB as required by the TFLite model.
    rgb_image = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)
    mp_image = mp.Image(image_format=mp.ImageFormat.SRGB, data=rgb_image)

    # Run hand landmarker using the model.
    detector.detect_async(mp_image, time.time_ns() // 1_000_000)

    # Show the FPS
    fps_text = 'FPS = {:.1f}'.format(FPS)
    text_location = (left_margin, row_size)
    current_frame = image
    cv2.putText(current_frame, fps_text, text_location,
                cv2.FONT_HERSHEY_DUPLEX,
                font_size, text_color, font_thickness, cv2.LINE_AA)

    gesture_count, gesture_anchor, gesture_scale = None, None, 0.0

    if DETECTION_RESULT:
      frame_height, frame_width = current_frame.shape[:2]

      for idx, hand_landmarks in enumerate(DETECTION_RESULT.hand_landmarks):
        # Convert normalized (0-1) landmark coordinates to pixel coordinates
        points = [(int(lm.x * frame_width), int(lm.y * frame_height))
                  for lm in hand_landmarks]

        # Draw the skeleton connections between landmarks
        for connection in HandLandmarksConnections.HAND_CONNECTIONS:
          cv2.line(current_frame, points[connection.start],
                    points[connection.end], (0, 255, 0), 2)

        # Draw a dot at each of the 21 landmarks
        for point in points:
          cv2.circle(current_frame, point, 4, (0, 0, 255), -1)

        # Label handedness (Left/Right) near the wrist (landmark 0)
        if DETECTION_RESULT.handedness:
          handedness_label = DETECTION_RESULT.handedness[idx][0].category_name
          # Frame is mirroed at line 120 reverse for this camera mounting 
          handedness_label = {'Left':'Right', 'Right':'Left'}[handedness_label]
          wrist_x, wrist_y = points[0]
          cv2.putText(current_frame, handedness_label,
                      (wrist_x, max(wrist_y - 20, 0)),
                      cv2.FONT_HERSHEY_DUPLEX, 1, handedness_text_color, 1,
                      cv2.LINE_AA)

      gesture_count, gesture_anchor, gesture_scale = gestureRecognition.classify(
          DETECTION_RESULT.hand_landmarks, frame_width, frame_height)

    # Fed every frame, including ones with no hand - that is what re-arms it.
    fired_floor = latch.update(gesture_count, gesture_anchor, gesture_scale,
                               time.monotonic())
    if fired_floor is not None:
      requester.request(fired_floor)

    latch_state, _, _ = latch.status()
    hud_text = 'GESTURE: {}  {}  [{}]'.format(
        gesture_count if gesture_count else '--', latch_state, requester.note)
    cv2.putText(current_frame, hud_text, (left_margin, row_size + 40),
                cv2.FONT_HERSHEY_DUPLEX, 0.7, handedness_text_color, 1,
                cv2.LINE_AA)

    cv2.imshow('hand_landmarker', current_frame)

    # Stop the program if the ESC key is pressed.
    if cv2.waitKey(1) == 27:
      break

  requester.stop()
  detector.close()
  picam2.stop()
  cv2.destroyAllWindows()


def main():
  parser = argparse.ArgumentParser(
      formatter_class=argparse.ArgumentDefaultsHelpFormatter)
  parser.add_argument(
      '--model',
      help='Name of hand landmarker model.',
      required=False,
      default='hand_landmarker.task')
  parser.add_argument(
      '--numHands',
      help='Max number of hands that can be detected by the landmarker.',
      required=False,
      default=1
      )
  parser.add_argument(
      '--minHandDetectionConfidence',
      help='The minimum confidence score for hand detection to be considered '
      'successful.',
      required=False,
      default=0.5)
  parser.add_argument(
      '--minHandPresenceConfidence',
      help='The minimum confidence score of hand presence score in the hand '
      'landmark detection.',
      required=False,
      default=0.5)
  parser.add_argument(
      '--minTrackingConfidence',
      help='The minimum confidence score for the hand tracking to be '
      'considered successful.',
      required=False,
      default=0.5)
  parser.add_argument(
      '--cameraId',
      help='Picamera2 camera_num (0 for a single Camera Module).',
      required=False,
      default=0)
  parser.add_argument(
      '--frameWidth',
      help='Width of frame to capture from camera.',
      required=False,
      default=1280)
  parser.add_argument(
      '--frameHeight',
      help='Height of frame to capture from camera.',
      required=False,
      default=960)
  args = parser.parse_args()

  run(args.model, int(args.numHands), float(args.minHandDetectionConfidence),
      float(args.minHandPresenceConfidence), float(args.minTrackingConfidence),
      int(args.cameraId), int(args.frameWidth), int(args.frameHeight))


if __name__ == '__main__':
  main()



