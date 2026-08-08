# Install python Dependencies
# Run this from inside the venv:
#   python3 -m venv handenv --system-site-packages
#   source handenv/bin/activate
#   sh setup.sh
python3 -m pip install pip --upgrade
python3 -m pip install -r requirements.txt

# -nc: don't re-download if the model is already here. The original script had
# no such flag, so re-running it produced a duplicate hand_landmarker.task.1.
wget -nc -q https://storage.googleapis.com/mediapipe-models/hand_landmarker/hand_landmarker/float16/1/hand_landmarker.task
