#!/usr/bin/python

from flask import Flask, request
import threading
import numpy as np
import face_recognition
import os


xres = 160
yres = 120
allowed_people_dir = "your_dir_with_faces"
allowed_people = []
allowed_people_names = []

app = Flask(__name__)

allowed_dir = os.fsencode(allowed_people_dir)
for file in os.listdir(allowed_dir):
    filename = os.fsdecode(file)
    allowed_person = face_recognition.load_image_file(allowed_people_dir + filename)
    encodings = face_recognition.face_encodings(allowed_person)
    if encodings:
        allowed_people.append(encodings[0])
        allowed_people_names.append(filename[:filename.find(".")])

print("Allowed people are ", allowed_people_names)


def recognize_compare_faces(rgb_array):
    global allowed_people, allowed_people_labels 
    rgb_array = np.ascontiguousarray(rgb_array)
    face_locations = face_recognition.face_locations(rgb_array)
    face_encodings = face_recognition.face_encodings(rgb_array, face_locations)

    frame_names = []
    for face_encoding in face_encodings:
        matches = face_recognition.compare_faces(allowed_people, face_encoding)

        if True in matches:
            name = allowed_people_names[matches.index(True)]
            frame_names.append(name)

    # if found faces on frame but no allowed faces
    if not frame_names and face_encodings:
        print("Found foreign faces")
    elif frame_names and face_encodings:
        print("Found: ", frame_names)
    elif not face_encodings:
        print("No faces found")


def rgb565_to_rgb888(rgb565):
    R5 = (rgb565 & 0b1111100000000000) >> 11
    G6 = (rgb565 & 0b0000011111100000) >> 5
    B5 = rgb565 & 0b0000000000011111
    r = ( R5 * 527 + 23 ) >> 6
    g = ( G6 * 259 + 33 ) >> 6
    b = ( B5 * 527 + 23 ) >> 6
    return r, g, b


def process_image(data):
    rgb888_data = []
    for i in range(0, len(data), 2):
        rgb888_data.append(rgb565_to_rgb888(data[i + 1] << 8 | data[i]))

    rgb_array = np.array(rgb888_data, dtype=np.uint8).reshape((yres, xres, 3))
    rgb_array = np.rot90(rgb_array)
    # output_filename = f'img.png'
    # image_n = Image.fromarray(rgb_array, mode='RGB')
    # image_n.save(output_filename)
    recognize_compare_faces(rgb_array)


@app.route('/recognize', methods=['POST'])
def upload():
    data = list(request.data)
    threading.Thread(target=process_image, args=(data,)).start()
    return "Frame received", 200


if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)

