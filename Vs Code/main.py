import cv2
import requests
import time
from ultralytics import YOLO

# ESP32 IP address
ESP32_IP = "http://192.168.4.1"

# Load YOLO model
model = YOLO("yolov8n.pt")

# Open camera
cap = cv2.VideoCapture(0)

last_send = 0

while True:
    ret, frame = cap.read()

    if not ret:
        break

    # Run YOLO detection
    results = model(frame, verbose=False)

    # Count only people (class 0 in YOLO is person)
    people_count = 0

    for result in results:
        for box in result.boxes:
            if int(box.cls) == 0:
                people_count += 1

                # Draw box around person
                x1, y1, x2, y2 = map(int, box.xyxy[0])

                cv2.rectangle(
                    frame,
                    (x1, y1),
                    (x2, y2),
                    (0, 255, 0),
                    2
                )

                cv2.putText(
                    frame,
                    "Person",
                    (x1, y1 - 10),
                    cv2.FONT_HERSHEY_SIMPLEX,
                    0.5,
                    (0, 255, 0),
                    2
                )

    # Display count on frame
    cv2.putText(
        frame,
        f"People: {people_count}",
        (10, 30),
        cv2.FONT_HERSHEY_SIMPLEX,
        1,
        (0, 255, 0),
        2
    )

    # Send command to ESP32 every 1 second
    current_time = time.time()

    if current_time - last_send >= 1:
        try:
            if people_count == 0:
                requests.get(
                    f"{ESP32_IP}/relay_off",
                    timeout=1
                )
                print("Room empty - Relay OFF")
            else:
                requests.get(
                    f"{ESP32_IP}/relay_on?count={people_count}",
                    timeout=1
                )
                print(f"{people_count} people - Relay ON")

            last_send = current_time

        except:
            print("ESP32 not reachable")

    cv2.imshow("Smart Classroom - YOLO", frame)

    if cv2.waitKey(1) & 0xFF == ord("q"):
        break

cap.release()
cv2.destroyAllWindows()