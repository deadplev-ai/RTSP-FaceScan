#pip install tensorflow-gpu

print("")
print("*********************************************")
print("FOR MAXIMUM PERFORMANCE: INSTALL TENSORFLOW GPU")
print("IT WILL FUNCTION WITHOUT IT, BUT WILL BE SLOWER")
print("*********************************************")
print("")

from deepface import DeepFace
import tkinter as tk
from PIL import Image, ImageTk
import cv2
import threading
import numpy as np
import tensorflow as tf

# Ensure TensorFlow uses the GPU
physical_devices = tf.config.experimental.list_physical_devices('GPU')
if physical_devices:
    try:
        tf.config.experimental.set_memory_growth(physical_devices, True)
        print("GPU is available and configured.")
    except RuntimeError as e:
        print(e)

# Variables Config
FRAME_WIDTH = 1280
FRAME_HEIGHT = 720
TOTAL_COUNT = 3 # Analyse every X Frames
rtsp_url = "rtsp://root:vvtk1234@192.168.3.206:554/live1s1.sdp?tcp&buffer_size=4096"

# Load OpenCV's pre-trained face detector
face_cascade = cv2.CascadeClassifier(cv2.data.haarcascades + 'haarcascade_frontalface_default.xml')

# Video Update Function
count = 0
bounding_boxes = []
analysis_thread = None

def analyze_frame(frame):
    global bounding_boxes
    try:
        analysis = DeepFace.analyze(frame, actions=['age', 'gender', 'emotion'], enforce_detection=True)
        
        # Update bounding boxes and labels
        bounding_boxes = []
        for face in analysis:
            region = face.get('region', {})
            if region:
                x, y, w, h = region['x'], region['y'], region['w'], region['h']
                text = f"{face['dominant_gender']}, {face['age']}, {face['dominant_emotion']}"
                bounding_boxes.append((x, y, w, h, text))
                print(text)
            else:
                print("No region found for face:", face)
    except Exception as e:
        Balls = 1 # Anti Error
        #print(f"Error in analysis: {e}")

def frame_update():
    global count, analysis_thread, bounding_boxes
    try:
        # Skip frames to get the most recent one for both streams
        for _ in range(5):  # Adjust the range as needed to skip more frames
            ret_viewer, frame_viewer = cap_viewer.read()
            ret_analysis, frame_analysis = cap_analysis.read()
        
        if ret_viewer and ret_analysis:
            scale_factor = 0.75
            window_width = root.winfo_width()
            window_height = root.winfo_height()

            scaled_width = int(window_width * scale_factor)
            scaled_height = int(window_height * scale_factor)

            if scaled_width > 0 and scaled_height > 0:
                frame_viewer = cv2.resize(frame_viewer, (scaled_width, scaled_height))
                frame_analysis = cv2.resize(frame_analysis, (scaled_width, scaled_height))
                
                # Quick face detection
                gray_frame = cv2.cvtColor(frame_analysis, cv2.COLOR_BGR2GRAY)
                faces = face_cascade.detectMultiScale(gray_frame, scaleFactor=1.05, minNeighbors=3, minSize=(30, 30))

                # Only analyze frame if faces are detected
                if len(faces) > 0:
                    if analysis_thread is None or not analysis_thread.is_alive():
                        if count >= TOTAL_COUNT:
                            analysis_thread = threading.Thread(target=analyze_frame, args=(frame_analysis,))
                            analysis_thread.start()
                            count = 0
                        else:
                            count += 1
                else:
                    # Clear bounding boxes if no faces are detected
                    bounding_boxes = []

                # Draw bounding boxes and labels on the viewer frame
                for (x, y, w, h, text) in bounding_boxes:
                    cv2.rectangle(frame_viewer, (x, y), (x+w, y+h), (0, 255, 0), 2)
                    cv2.putText(frame_viewer, text, (x, y-10), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 2)

                # Convert the viewer frame to RGB and update the GUI
                img = cv2.cvtColor(frame_viewer, cv2.COLOR_BGR2RGB)
                img = Image.fromarray(img)
                imgtk = ImageTk.PhotoImage(image=img)

                label.imgtk = imgtk
                label.configure(image=imgtk)
        
        root.after(100, frame_update) # Update Loop
    except Exception as e:
        Balls2 = 1
        #print(f"Error in frame update: {e}")

def warm_up_model():
    # Create a dummy image for warm-up
    dummy_image = np.zeros((224, 224, 3), dtype=np.uint8)
    try:
        DeepFace.analyze(dummy_image, actions=['age', 'gender', 'race', 'emotion'], enforce_detection=False)
        print("Model warm-up complete.")
    except Exception as e:
        print(f"Error during model warm-up: {e}")

# Main GUI
root = tk.Tk()
root.title("DeepFace RTSP Stream (Vivotek)")
root.geometry(f"{FRAME_WIDTH}x{FRAME_HEIGHT}")

cap_viewer = cv2.VideoCapture(rtsp_url)
cap_analysis = cv2.VideoCapture(rtsp_url)

if not cap_viewer.isOpened() or not cap_analysis.isOpened():
    print("Error: Unable to open the RTSP stream.")
    exit()

label = tk.Label(root)
label.pack(fill=tk.BOTH, expand=True)

cap_viewer.set(cv2.CAP_PROP_BUFFERSIZE, 1) # Optimisation
cap_analysis.set(cv2.CAP_PROP_BUFFERSIZE, 1) # Optimisation

# Warm-up the model
warm_up_model()

# Start the frame update loop
frame_update()

# Run the GUI main loop
root.mainloop()

# Release the video capture when the GUI is closed
cap_viewer.release()
cap_analysis.release()
