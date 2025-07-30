
#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <string>
#include <thread>
#include <variant>
#include <any>
#include <map>
#include <iomanip>
#include <fstream>
#include <sys/mman.h>  // For mmap, munmap, PROT_READ, PROT_WRITE, MAP_SHARED
#include <fcntl.h> 
#include <unistd.h>

#include <libcamera/transform.h>
#include <libcamera/libcamera.h>
#include <opencv2/opencv.hpp>
#include <libcamera/base/span.h>
#include <libcamera/camera.h>
#include <libcamera/camera_manager.h>
#include <libcamera/control_ids.h>
#include <libcamera/controls.h>
#include <libcamera/formats.h>
#include <libcamera/framebuffer_allocator.h>
#include <libcamera/property_ids.h>

#include <opencv2/imgproc.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>

using namespace libcamera;
std::unique_ptr<CameraManager> manager;
std::shared_ptr<Camera> camera;
std::vector<std::unique_ptr<Request>> requests;

Stream *streamView;
Stream *streamStill;
Stream *streamVideo;
Stream *streamRaw;

std::map<int, std::pair<void *, unsigned int>> mappedBuffersView_;
std::map<int, std::pair<void *, unsigned int>> mappedBuffersStill_;
std::map<int, std::pair<void *, unsigned int>> mappedBuffersVideo_;
std::map<int, std::pair<void *, unsigned int>> mappedBuffersRaw_;

std::queue<Request *> requestQueue;

/*

------------------  YUYV 1plane  ------------------
    const FrameBuffer::Plane &plane = buffer->planes()[0];
    void *data = mappedBuffersView_[plane.fd.get()].first;
    cv::Mat yuyv(height, width, CV_8UC2, data, stride);
    cv::Mat bgr;
    cv::cvtColor(yuyv, bgr, cv::COLOR_YUV2BGR_YUY2);
    cv::imwrite("YUYV.png", bgr);

------------------ RGB888 1plane ------------------
    const FrameBuffer::Plane &plane = buffer->planes()[0];
    void *data = mappedBuffersStill_[plane.fd.get()].first;
    if (!data) {
        std::cerr << "Null data pointer for still stream!" << std::endl;
        continue;
    }
    cv::Mat image(height, width, CV_8UC3, data, stride);
    cv::Mat copied = image.clone();
    cv::imwrite("RGB888.png", copied);

------------------ SBGGR10 1plane ------------------
    const FrameBuffer::Plane &plane = buffer->planes()[0];
    void *data = mappedBuffersRaw_[plane.fd.get()].first;
    const FrameMetadata::Plane &meta = buffer->metadata().planes()[0];
    int length = std::min(meta.bytesused, plane.length);
    cv::Mat raw16(height, stride / 2, CV_16UC1, data); // divide stride because it's in bytes
    cv::Mat cropped = raw16(cv::Rect(0, 0, width, height));
    cv::Mat bayer10, bayer8, rgb;
    cv::bitwise_and(cropped, 0x03FF, bayer10); // apply 10-bit mask
    bayer10.convertTo(bayer8, CV_8UC1, 255.0 / 1023.0); // scale 10-bit to 8-bit
    cv::cvtColor(bayer8, rgb, cv::COLOR_BayerBG2RGB); // for SBGGR10 use BG2RGB
    cv::imwrite("SBGGR10.png", rgb);
*/

void requestComplete(Request *request){
	if (request->status() == Request::RequestCancelled)
		return;

    // std::cout << std::endl << "Request completed: " << request->toString() << std::endl;

    requestQueue.push(request);
/*
    const Request::BufferMap &buffers = request->buffers();
    for (auto &[stream, buffer] : buffers) {
        int width = stream->configuration().size.width;
        int height = stream->configuration().size.height;
        int stride = stream->configuration().stride;

        if (stream == streamView) {
            std::cout << "View Stream: " << width << ", " << height << ", tride: " << stride << ", sizeBuffer: " << buffer->planes().size() <<std::endl;
        }else if (stream == streamVideo) {
            std::cout << "Video Stream: " << width << ", " << height << ", tride: " << stride << ", sizeBuffer: " << buffer->planes().size() <<std::endl;
        }else if (stream == streamStill) {
            const FrameBuffer::Plane &plane = buffer->planes()[0];
            void *data = mappedBuffersStill_[plane.fd.get()].first;
            if (!data) {
                std::cerr << "Null data pointer for still stream!" << std::endl;
                continue;
            }
            cv::Mat image(height, width, CV_8UC3, data, stride);
            cv::Mat copied = image.clone();
            cv::imwrite("RGB888.png", copied);
            // std::cout << "Still Stream: " << width << ", " << height << ", tride: " << stride << ", sizeBuffer: " << buffer->planes().size() <<std::endl;
        }else if (stream == streamRaw) {
            const FrameBuffer::Plane &plane = buffer->planes()[0];
            void *data = mappedBuffersRaw_[plane.fd.get()].first;
            const FrameMetadata::Plane &meta = buffer->metadata().planes()[0];
            int length = std::min(meta.bytesused, plane.length);
            cv::Mat raw16(height, stride / 2, CV_16UC1, data); // divide stride because it's in bytes
            cv::Mat cropped = raw16(cv::Rect(0, 0, width, height));
            cv::Mat bayer10, bayer8, rgb;
            cv::bitwise_and(cropped, 0x03FF, bayer10); // apply 10-bit mask
            bayer10.convertTo(bayer8, CV_8UC1, 255.0 / 1023.0); // scale 10-bit to 8-bit
            cv::cvtColor(bayer8, rgb, cv::COLOR_BayerBG2RGB); // for SBGGR10 use BG2RGB
            cv::imwrite("SBGGR10.png", rgb);

            // std::cout << "Raw Stream: " << width << ", " << height << ", tride: " << stride << ", sizeBuffer: " << buffer->planes().size() <<std::endl;
        }
    }
*/
/*
    const ControlList &requestMetadata = request->metadata();
	for (const auto &ctrl : requestMetadata) {
		const ControlId *id = controls::controls.at(ctrl.first);
		const ControlValue &value = ctrl.second;

		std::cout << "\t" << id->name() << " = " << value.toString() << std::endl;
	}

    const Request::BufferMap &buffers = request->buffers();
	for (auto bufferPair : buffers) {
		// (Unused) Stream *stream = bufferPair.first;
		FrameBuffer *buffer = bufferPair.second;
		const FrameMetadata &metadata = buffer->metadata();

		std::cout << " seq: " << std::setw(6) << std::setfill('0') << metadata.sequence
			  << " timestamp: " << metadata.timestamp
			  << " bytesused: ";

		unsigned int nplane = 0;
		for (const FrameMetadata::Plane &plane : metadata.planes()){
			std::cout << plane.bytesused;
			if (++nplane < metadata.planes().size())
				std::cout << "/";
		}

		std::cout << std::endl;
	}
*/
    // request->reuse(Request::ReuseBuffers);
	// camera->queueRequest(request);
}

std::mutex free_requests_mutex_;
void imageHandle(){
    std::lock_guard<std::mutex> lock(free_requests_mutex_);
    int cnt = 0;
    while(1){
        if (!requestQueue.empty()){
            Request *request = requestQueue.front();
            const Request::BufferMap &buffers = request->buffers();
            for (auto &[stream, buffer] : buffers) {
                int width = stream->configuration().size.width;
                int height = stream->configuration().size.height;
                int stride = stream->configuration().stride;

                if (stream == streamView) {
                    std::cout << "View Stream: " << width << ", " << height << ", tride: " << stride << ", sizeBuffer: " << buffer->planes().size() <<std::endl;
                }else if (stream == streamVideo) {
                    std::cout << "Video Stream: " << width << ", " << height << ", tride: " << stride << ", sizeBuffer: " << buffer->planes().size() <<std::endl;
                }else if (stream == streamStill) {
                    const FrameBuffer::Plane &plane = buffer->planes()[0];
                    void *data = mappedBuffersStill_[plane.fd.get()].first;
                    if (!data) {
                        std::cerr << "Null data pointer for still stream!" << std::endl;
                        continue;
                    }
                    cv::Mat image(height, width, CV_8UC3, data, stride);
                    cv::Mat copied = image.clone();
                    cv::imwrite("RGB888.png", copied);
                    std::cout << "Still Stream: " << width << ", " << height << ", tride: " << stride << ", sizeBuffer: " << buffer->planes().size() <<std::endl;
                }else if (stream == streamRaw) {
                    const FrameBuffer::Plane &plane = buffer->planes()[0];
                    void *data = mappedBuffersRaw_[plane.fd.get()].first;
                    const FrameMetadata::Plane &meta = buffer->metadata().planes()[0];
                    int length = std::min(meta.bytesused, plane.length);
                    cv::Mat raw16(height, stride / 2, CV_16UC1, data); // divide stride because it's in bytes
                    cv::Mat cropped = raw16(cv::Rect(0, 0, width, height));
                    cv::Mat bayer10, bayer8, rgb;
                    cv::bitwise_and(cropped, 0x03FF, bayer10); // apply 10-bit mask
                    bayer10.convertTo(bayer8, CV_8UC1, 255.0 / 1023.0); // scale 10-bit to 8-bit
                    cv::cvtColor(bayer8, rgb, cv::COLOR_BayerBG2RGB); // for SBGGR10 use BG2RGB
                    std::string filename = "SBGGR10_" + std::to_string(cnt++) + ".png";
                    cv::imwrite(filename, rgb);
                    std::cout << "Raw Stream: " << width << ", " << height << ", tride: " << stride << ", sizeBuffer: " << buffer->planes().size() <<std::endl;
                }
            }
            usleep(100000);
            requestQueue.pop();

            request->reuse(Request::ReuseBuffers);
	        camera->queueRequest(request);
        }
    }
}

int main() {    
    manager = std::make_unique<CameraManager>();
    manager->start();
    std::cout << "Camera manager started" << std::endl;

    if (manager->cameras().empty()) {
        std::cerr << "No cameras found!" << std::endl;
        return 1;
    }

    camera = manager->cameras()[0];
    std::cout << "Found camera: " << camera->id() << std::endl;

    if (camera->acquire()) {
        std::cerr << "Failed to acquire camera" << std::endl;
        return 1;
    }
    std::cout << "Camera acquired successfully" << std::endl;

    // Configure two streams
    std::unique_ptr<CameraConfiguration> configuration =
		camera->generateConfiguration( { StreamRole::StillCapture, StreamRole::Raw } ); //Viewfinder //StillCapture //Raw
    for (size_t i = 0; i < configuration->size(); ++i) {
        StreamConfiguration &config = configuration->at(i);
        std::cout << "Stream " << i << "default configuration: " << config.toString() << std::endl;
    }

    //Change stream config
    configuration->at(0).size.width = 1920;
    configuration->at(0).size.height = 1080;
    configuration->at(0).pixelFormat = formats::RGB888; //YUV420, RGB888, MJPEG
    configuration->at(0).colorSpace = ColorSpace::Sycc;

    configuration->at(1).size.width = 4608;
    configuration->at(1).size.height = 2592;
    configuration->at(1).pixelFormat = formats::SRGGB10; //SRGGB10_CSI2P //SRGGB10

    // Validate configuration
    configuration->validate();

    if (camera->configure(configuration.get())) {
        std::cerr << "Configuration failed" << std::endl;
        return 1;
    }
    std::cout << "Camera configured successfully" << std::endl;

    for (const StreamConfiguration &cfg : *configuration) {
        std::cout << "Size: " << cfg.size.toString()
                << ", pixelFormat: " << cfg.pixelFormat.toString()
                << ", stride: " << cfg.stride << std::endl;
    }

    // Allocator
    FrameBufferAllocator *allocator = new FrameBufferAllocator(camera);
    for (StreamConfiguration &config : *configuration){
        int ret = allocator->allocate(config.stream());
        if (ret < 0) {
            std::cerr << "Can't allocate buffers" << std::endl;
            return -1;
        }

        size_t allocated = allocator->buffers(config.stream()).size();
        std::cout << "Allocated " << allocated << " buffers for stream" << std::endl;
    }


    // Frame Capture
    // streamView = configuration->at(0).stream();
    // streamVideo = configuration->at(1).stream();
    streamStill = configuration->at(0).stream();
    streamRaw = configuration->at(1).stream();
    

    //StreamConfiguration const &cfg0 = streamView->configuration();
    //StreamConfiguration const &cfg1 = streamVideo->configuration();
    // StreamConfiguration const &cfg2 = streamStill->configuration();
    // StreamConfiguration const &cfg3 = streamRaw->configuration();
    //std::cout << "Size0: " << cfg0.size.width << ", " << cfg0.size.height << ", stride: " << cfg0.stride << std::endl;
    //std::cout << "Size1: " << cfg1.size.width << ", " << cfg1.size.height << ", stride: " << cfg1.stride << std::endl;
    // std::cout << "Size1: " << cfg2.size.width << ", " << cfg2.size.height << ", stride: " << cfg2.stride << std::endl;
    // std::cout << "Size1: " << cfg3.size.width << ", " << cfg3.size.height << ", stride: " << cfg3.stride << std::endl;

    // const std::vector<std::unique_ptr<FrameBuffer>> &buffersView = allocator->buffers(streamView);
    // const std::vector<std::unique_ptr<FrameBuffer>> &buffersVideo = allocator->buffers(streamVideo);
    const std::vector<std::unique_ptr<FrameBuffer>> &buffersStill = allocator->buffers(streamStill);
    const std::vector<std::unique_ptr<FrameBuffer>> &buffersRaw = allocator->buffers(streamRaw);

    // for (unsigned int i = 0; i < buffersView.size(); ++i) {
	// 	std::unique_ptr<Request> request = camera->createRequest();
	// 	if (!request){
	// 		std::cerr << "Can't create request" << std::endl;
	// 		return EXIT_FAILURE;
	// 	}

	// 	const std::unique_ptr<FrameBuffer> &buffer = buffersView[i];
	// 	int ret = request->addBuffer(streamView, buffer.get());
	// 	if (ret < 0){
	// 		std::cerr << "Can't set buffer for request" << std::endl;
	// 		return EXIT_FAILURE;
	// 	}

    //     for (const FrameBuffer::Plane &plane : buffer->planes()) {
    //         void *memory = mmap(NULL, plane.length, PROT_READ, MAP_SHARED, plane.fd.get(), 0);
    //         mappedBuffersView_[plane.fd.get()] = std::make_pair(memory, plane.length);
    //     }

	// 	requests.push_back(std::move(request));
	// }

    // for (unsigned int i = 0; i < buffersVideo.size(); ++i) {
	// 	std::unique_ptr<Request> request = camera->createRequest();
	// 	if (!request){
	// 		std::cerr << "Can't create request" << std::endl;
	// 		return EXIT_FAILURE;
	// 	}

	// 	const std::unique_ptr<FrameBuffer> &buffer = buffersVideo[i];
	// 	int ret = request->addBuffer(streamVideo, buffer.get());
	// 	if (ret < 0){
	// 		std::cerr << "Can't set buffer for request" << std::endl;
	// 		return EXIT_FAILURE;
	// 	}

    //     for (const FrameBuffer::Plane &plane : buffer->planes()) {
    //         void *memory = mmap(NULL, plane.length, PROT_READ, MAP_SHARED, plane.fd.get(), 0);
    //         mappedBuffersVideo_[plane.fd.get()] = std::make_pair(memory, plane.length);
    //     }

	// 	requests.push_back(std::move(request));
	// }

    for (unsigned int i = 0; i < buffersStill.size(); ++i) {
		std::unique_ptr<Request> request = camera->createRequest();
		if (!request){
			std::cerr << "Can't create request" << std::endl;
			return EXIT_FAILURE;
		}

		const std::unique_ptr<FrameBuffer> &buffer = buffersStill[i];
		int ret = request->addBuffer(streamStill, buffer.get());
		if (ret < 0){
			std::cerr << "Can't set buffer for request" << std::endl;
			return EXIT_FAILURE;
		}

        for (const FrameBuffer::Plane &plane : buffer->planes()) {
            void *memory = mmap(NULL, plane.length, PROT_READ, MAP_SHARED, plane.fd.get(), 0);
            mappedBuffersStill_[plane.fd.get()] = std::make_pair(memory, plane.length);
        }

		requests.push_back(std::move(request));
	}

    for (unsigned int i = 0; i < buffersRaw.size(); ++i) {
		std::unique_ptr<Request> request = camera->createRequest();
		if (!request){
			std::cerr << "Can't create request" << std::endl;
			return EXIT_FAILURE;
		}

		const std::unique_ptr<FrameBuffer> &buffer = buffersRaw[i];
		int ret = request->addBuffer(streamRaw, buffer.get());
		if (ret < 0){
			std::cerr << "Can't set buffer for request" << std::endl;
			return EXIT_FAILURE;
		}

        for (const FrameBuffer::Plane &plane : buffer->planes()) {
            void *memory = mmap(NULL, plane.length, PROT_READ, MAP_SHARED, plane.fd.get(), 0);
            mappedBuffersRaw_[plane.fd.get()] = std::make_pair(memory, plane.length);
        }

		requests.push_back(std::move(request));
	}

    // Signal & Slot
    camera->requestCompleted.connect(requestComplete);

    std::thread t(imageHandle);
    // Start Capture
    camera->start();
    for (std::unique_ptr<Request> &request : requests)
		camera->queueRequest(request.get());


    while(true);

    for (auto &iter : mappedBuffersView_){
        std::pair<void *, unsigned int> pair_ = iter.second;
		munmap(std::get<0>(pair_), std::get<1>(pair_));
	}
    mappedBuffersView_.clear();
    for (auto &iter : mappedBuffersVideo_){
        std::pair<void *, unsigned int> pair_ = iter.second;
		munmap(std::get<0>(pair_), std::get<1>(pair_));
	}
    mappedBuffersVideo_.clear();
    for (auto &iter : mappedBuffersStill_){
        std::pair<void *, unsigned int> pair_ = iter.second;
		munmap(std::get<0>(pair_), std::get<1>(pair_));
	}
    mappedBuffersStill_.clear();
    for (auto &iter : mappedBuffersRaw_){
        std::pair<void *, unsigned int> pair_ = iter.second;
		munmap(std::get<0>(pair_), std::get<1>(pair_));
	}
    mappedBuffersRaw_.clear();

    camera->stop();
	delete allocator;
	allocator = nullptr;
    camera->release();
    camera.reset();
    manager->stop();

    return 0;
}
