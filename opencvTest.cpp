/*
#include <opencv2/opencv.hpp>
#include <iostream>

using namespace cv;
using namespace std;

// Number of frames to pass before changing the frame to compare the current
// frame against
const int FRAMES_TO_PERSIST = 10;

// Minimum boxed area for a detected motion to count as actual motion
// Use to filter out noise or small objects
const int MIN_SIZE_FOR_MOVEMENT = 2000;

// Minimum length of time where no motion is detected it should take
//(in program cycles) for the program to declare that there is no movement
const int MOVEMENT_DETECTED_PERSISTENCE = 10;


int main() {
    // Create capture object
    string video_path = "D:\\Batlab\\Final-MP4.mp4"; // Replace with your video file path
    VideoCapture cap(video_path);

    if (!cap.isOpened()) {
        cerr << "Error opening video stream or file" << endl;
        return -1;
    }

    // Init frame variables
    Mat first_frame, next_frame;

    // Init display font and timeout counters
    int delay_counter = 0;
    int movement_persistent_counter = 0;

    while (true) {
        // Set transient motion detected as false
        bool transient_movement_flag = false;

        // Read frame
        Mat frame;
        bool ret = cap.read(frame);
        string text = "Unoccupied";

        // If there's an error in capturing or end of video
        if (!ret) {
            cout << "End of video or CAPTURE ERROR" << endl;
            break;
        }

        // Resize and save a greyscale version of the image
        resize(frame, frame, Size(750, frame.rows * 750 / frame.cols));
        Mat gray;
        cvtColor(frame, gray, COLOR_BGR2GRAY);

        // Blur it to remove camera noise (reducing false positives)
        GaussianBlur(gray, gray, Size(21, 21), 0);

        // If the first frame is nothing, initialise it
        if (first_frame.empty()) {
            first_frame = gray;
        }

        delay_counter += 1;

        // Otherwise, set the first frame to compare as the previous frame
        // But only if the counter reaches the appropriate value
        if (delay_counter > FRAMES_TO_PERSIST) {
            delay_counter = 0;
            first_frame = next_frame;
        }

        // Set the next frame to compare (the current frame)
        next_frame = gray;

        // Compare the two frames, find the difference
        Mat frame_delta;
        absdiff(first_frame, next_frame, frame_delta);
        Mat thresh;
        threshold(frame_delta, thresh, 25, 255, THRESH_BINARY);

        // Fill in holes via dilate(), and find contours of the thresholds
        dilate(thresh, thresh, Mat(), Point(-1, -1), 2);
        vector<vector<Point>> contours;
        findContours(thresh.clone(), contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

        // Loop over the contours
        for (size_t i = 0; i < contours.size(); i++) {
            // Save the coordinates of all found contours
            Rect bounding_box = boundingRect(contours[i]);

            // If the contour is too small, ignore it, otherwise, there's transient movement
            if (contourArea(contours[i]) > MIN_SIZE_FOR_MOVEMENT) {
                transient_movement_flag = true;

                // Draw a rectangle around big enough movements
                rectangle(frame, bounding_box.tl(), bounding_box.br(), Scalar(0, 255, 0), 2);
            }
        }

        // The moment something moves momentarily, reset the persistent movement timer.
        if (transient_movement_flag) {
            movement_persistent_counter = MOVEMENT_DETECTED_PERSISTENCE;
        }

        // As long as there was a recent transient movement, say a movement was detected
        if (movement_persistent_counter > 0) {
            text = "Movement Detected " + to_string(movement_persistent_counter);
            movement_persistent_counter -= 1;
        }
        else {
            text = "No Movement Detected";
        }

        // Print the text on the screen, and display the raw and processed video feeds
        putText(frame, text, Point(10, 35), FONT_HERSHEY_SIMPLEX, 0.75, Scalar(255, 255, 255), 2, LINE_AA);

        // For if you want to show the individual video frames
        //imshow("frame", frame);
        //imshow("delta", frame_delta);

        // Convert the frame_delta to color for splicing
        cvtColor(frame_delta, frame_delta, COLOR_GRAY2BGR);

        // Splice the two video frames together to make one long horizontal one
        Mat display_frame;
        hconcat(vector<Mat>{frame_delta, frame}, display_frame);
        imshow("frame", display_frame);

        // Interrupt trigger by pressing q to quit the open CV program
        char ch = (char)waitKey(1);
        if (ch == 'q' || ch == 'Q') {
            break;
        }
    }

    // Cleanup when closed
    waitKey(0);
    destroyAllWindows();
    cap.release();

    return 0;
}
*/