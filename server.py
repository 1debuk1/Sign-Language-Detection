from flask import Flask, request, jsonify
import numpy as np
import cv2
import os
from main import SignLanguageDetector

app = Flask(__name__)
detector = SignLanguageDetector()

@app.route('/detect', methods=['POST'])
def detect():
    if 'image' not in request.files:
        return jsonify({'error': 'No image uploaded'}), 400
    file = request.files['image']
    img_bytes = np.frombuffer(file.read(), np.uint8)
    frame = cv2.imdecode(img_bytes, cv2.IMREAD_COLOR)
    if frame is None:
        return jsonify({'error': 'Invalid image'}), 400

    # Run recognition
    alphabet = detector.detect_hand_and_alphabet(frame)
    
    # Get confidence
    try:
        if detector.model is None:
            print("Model not loaded, using fallback detection")
            confidence = 0.0
        else:
            processed_image = detector.preprocess_image(frame)
            predictions = detector.model.predict(processed_image, verbose=0)
            predicted_class = np.argmax(predictions[0])
            confidence = predictions[0][predicted_class]
            
            # Get all predictions for debugging
            all_predictions = []
            if detector.labels is not None:
                for i, pred in enumerate(predictions[0]):
                    if pred > 0.1:  # Show predictions with >10% confidence
                        all_predictions.append((detector.labels[i], float(pred)))
                all_predictions.sort(key=lambda x: x[1], reverse=True)
                
                print(f"Top predictions: {all_predictions[:3]}")
            else:
                print("Labels not loaded")
        
    except Exception as e:
        print(f"Error getting confidence: {e}")
        confidence = 0.0

    # Lower confidence threshold to detect more alphabets
    if confidence > 0.3:  # Reduced from 0.85 to 0.3
        print(f"Detected: {alphabet} (confidence: {confidence:.3f})")
        return jsonify({'alphabet': alphabet, 'confidence': float(confidence)})
    else:
        print(f"Low confidence detection: {alphabet} (confidence: {confidence:.3f})")
        return jsonify({'alphabet': 'nothing', 'confidence': float(confidence)})

if __name__ == '__main__':
    print("Starting Sign Language Recognition Server...")
    print("Supported alphabets: A-Z, del, nothing, space")
    app.run(host='0.0.0.0', port=xxxx)