#include "face.h"

int capture_face(cv::Mat &frame)
{
    cv::VideoCapture camera(0, cv::CAP_V4L2); // in linux check $ ls /dev/video0
    int retries = 0;
    while (!camera.isOpened())
    {
        if (retries > 5)
        {
            std::cout << "Failed to open camera." << std::endl;
            return 1;
        }
        std::cout << "Trying to open camera..." << std::endl;
        camera.open(0);
        sleep(1);
        retries++;
    }

    printf("Camera opened.\n");
    while (1)
    {
        // capture the next frame from the webcam
        camera >> frame;
        // show the image on the window
        cv::imshow("Facepass", frame);
        // wait for enter key to be pressed to stop
        const int key = cv::waitKey(10);
        // if the user presses 'Enter' or 'Esc', exit the loop
        if (key == 13 || key == 27)
            break;
    }
    return 0;
}

cv::Mat detect_face(const string &username, cv::Mat &frame)
{
    const string model = model_path(username, FACE_DETECTION);
    FaceDetection detector(model);
    std::vector<Detection> detectedImages = detector.inference(frame);
    if (detectedImages.empty())
        return cv::Mat();
    cv::Mat face = detectedImages[0].image;
    return face;
}

int add_face(const string &username)
{
    string faceImagePath = user_face_path(username);
    printf("Saving face to %s\n", faceImagePath.c_str());
    cv::Mat screenshot;
    int result = capture_face(screenshot);
    if (result != 0)
    {
        printf("Failed to capture face.\n");
        return 1;
    }
    cv::Mat face = detect_face(username, screenshot);
    if (face.empty())
    {
        printf("Failed to detect face.\n");
        return 1;
    }
    result = cv::imwrite(faceImagePath, face);
    if (result == 0)
    {
        printf("Face size dimension %d", face.size.dims());
        printf("Failed to save face.\n");
        return 1;
    }

    return 0;
}

int remove_face(const string &user) {
    return remove(user_face_path(user).c_str()) != 0;
}