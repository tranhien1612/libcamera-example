#include <iostream>
#include <pthread.h>

#include <lccv.hpp>
#include <libcamera_app.hpp>
#include <opencv2/opencv.hpp>
#include <gst/app/gstappsrc.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

//-llccv
//g++ main.cpp -o main `pkg-config --cflags --libs  libcamera  opencv4 gstreamer-1.0 gstreamer-app-1.0` -llccv
/*
Install:
    git clone https://github.com/kbarni/LCCV.git
    cd LCCV
    mkdir build && cd build
    cmake .. && make
    sudo make install
*/

// cv::Mat frame;
cv::Mat frame(1080, 1920, CV_8UC3, cv::Scalar(0, 0, 0));

void setVideoParam(lccv::PiCamera &cam){
    cam.options->video_width=1280;
    cam.options->video_height=720;
    cam.options->framerate=30;
    cam.options->verbose=true;
}

void setPhotoParam(lccv::PiCamera &cam){
    cam.options->video_width=4056;
    cam.options->video_height=3040;
    cam.options->framerate=30;
    cam.options->verbose=true;
}

std::string getTimestampedFilename(const std::string& extension = ".jpg") {
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << "image_"<< std::put_time(&tm, "%Y%m%d_%H%M%S") << extension;
    return oss.str();
}

/*
gst-launch-1.0 -v videotestsrc is-live=true ! x264enc tune=zerolatency bitrate=500 speed-preset=superfast ! rtph264pay config-interval=1 pt=96 ! udpsink host=192.168.1.61 port=5600
*/
std::string pipeline = "appsrc ! videoconvert ! video/x-raw,width=1280,height=720,framerate=30/1 ! x264enc tune=zerolatency bitrate=2000000 speed-preset=superfast ! rtph264pay config-interval=1 pt=96 ! udpsink host=192.168.2.1 port=5600";  
void* camera_handler(void *param){
    uint32_t num_cams = LibcameraApp::GetNumberCameras();
	std::cout << "Found " << num_cams << " cameras." << std::endl;
    int cnt = 0;

    cv::Mat image;
    lccv::PiCamera cam;
    cv::namedWindow("Image",cv::WINDOW_NORMAL);

    setVideoParam(cam);
    cam.startVideo();
    while(true){

        if (cam.getVideoFrame(image, 1000)){
            cv::imshow("Image",image);
            
            cv::VideoWriter writer(pipeline, 0, 30, image.size(), true);
            if (!writer.isOpened()) {
                std::cerr << "Error: Could not open GStreamer pipeline!" << std::endl;
            }
            writer.write(image);
        }
        if (cv::waitKey(1) == 27) break;
        /*
        if(cnt < 500){
            if (cam.getVideoFrame(image, 1000)){
                cv::imshow("Image",image);
            }
            if (cv::waitKey(1) == 27) break; 
        }else{
            cam.stopVideo();
            setPhotoParam(cam);
            if (cam.capturePhoto(image)){
                std::string filename = getTimestampedFilename();
                cv::imwrite(filename, image);
            }
            setVideoParam(cam);
            cam.startVideo();
            cnt = 0;
        }
        cnt ++;
        */
    }
    cam.stopVideo();
    cv::destroyWindow("Image");
    return nullptr;
}

void* stream_handler(void *param){
    std::cout << "Start stream via UDP" <<std::endl;

    std::string pipeline_str = "appsrc ! videoconvert ! x264enc tune=zerolatency bitrate=500 speed-preset=superfast ! rtph264pay config-interval=1 pt=96 ! udpsink host=192.168.1.61 port=5600";    
    
    while (frame.empty());
    cv::VideoWriter writer(pipeline_str, 0, 30, frame.size(), true);
    if (!writer.isOpened()) {
        std::cerr << "Error: Could not open GStreamer pipeline!" << std::endl;
    }
    while (true) {
        if (frame.empty()) break;
        writer.write(frame);

        if (cv::waitKey(1) == 27) break;
    }

    return nullptr;
}

int main() {

    pthread_t camThread;
    if( pthread_create( &camThread , NULL , camera_handler, nullptr) < 0){
        perror("could not create thread");
        return 0;
    }
    
    // pthread_t streamThread;
    // if( pthread_create( &streamThread , NULL , stream_handler, nullptr) < 0){
    //     perror("could not create thread");
    //     return 0;
    // }


    // pthread_join(streamThread, nullptr);
    pthread_join(camThread, nullptr);
    
    return 0;
}
