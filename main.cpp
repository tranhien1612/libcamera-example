#include <lccv.hpp>
#include <libcamera_app.hpp>
#include <opencv2/opencv.hpp>

//g++ main.cpp -o main `pkg-config --cflags --libs  libcamera  opencv4` -llccv

int main() {
	uint32_t num_cams = LibcameraApp::GetNumberCameras();
	std::cout << "Found " << num_cams << " cameras." << std::endl;

    cv::Mat image;
    lccv::PiCamera cam;
    cam.options->video_width=1920;
    cam.options->video_height=1080;
    cam.options->framerate=30;
    cam.options->verbose=true;
    cv::namedWindow("Image",cv::WINDOW_NORMAL);

    //photo
    // while(true){
    //     if(!cam.capturePhoto(image)){
    //         std::cout<<"Camera error"<<std::endl;
    //     }
    //     cv::imshow("Image",image);
    //     // cv::imwrite("test.jpg", image);
    //     if (cv::waitKey(1) == 27) break; 
    // }

    //video
    cam.startVideo();
    while(true){
        if (cam.getVideoFrame(image, 1000)){
			cv::imshow("Video",image);
		}
        if (cv::waitKey(1) == 27) break; 
    }
    cam.stopVideo();
    //
  
    cv::destroyWindow("Image");
    return 0;
}
