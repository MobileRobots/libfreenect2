/*
 * This file is part of the OpenKinect Project. http://www.openkinect.org
 *
 * Copyright (c) 2011 individual OpenKinect contributors. See the CONTRIB file
 * for details.
 *
 * This code is licensed to you under the terms of the Apache License, version
 * 2.0, or, at your option, the terms of the GNU General Public License,
 * version 2.0. See the APACHE20 and GPL2 files for the text of the licenses,
 * or the following URLs:
 * http://www.apache.org/licenses/LICENSE-2.0
 * http://www.gnu.org/licenses/gpl-2.0.txt
 *
 * If you redistribute this file in source form, modified or unmodified, you
 * may:
 *   1) Leave this header intact and distribute it under the same terms,
 *      accompanying it with the APACHE20 and GPL20 files, or
 *   2) Delete the Apache 2.0 clause and accompany it with the GPL2 file, or
 *   3) Delete the GPL v2 clause and accompany it with the APACHE20 file
 * In all cases you must keep the copyright notice intact and include a copy
 * of the CONTRIB file.
 *
 * Binary distributions must follow the binary distribution requirements of
 * either License.
 */


#include <iostream>
#include <signal.h>

#include <libfreenect2/opengl.h>

#include <opencv2/opencv.hpp>

#include <libfreenect2/libfreenect2.hpp>
#include <libfreenect2/frame_listener_impl.h>
#include <libfreenect2/threading.h>

#include <libfreenect2/packet_pipeline.h>

bool protonect_shutdown = false;

void sigint_handler(int s)
{
  protonect_shutdown = true;
}

int main(int argc, char *argv[])
{
  std::string program_path(argv[0]);
  size_t executable_name_idx = program_path.rfind("Protonect");

  std::string binpath = "/";

  if(executable_name_idx != std::string::npos)
  {
    binpath = program_path.substr(0, executable_name_idx);
  }

  glfwInit();

  libfreenect2::Freenect2 freenect2;
//  libfreenect2::Freenect2Device *dev = freenect2.openDefaultDevice();
  libfreenect2::Freenect2Device *dev = 0;
  for(int i = 1; i < argc; ++i)
  {
#ifdef LIBFREENECT2_WITH_OPENCL_SUPPORT
    if(strcmp(argv[i], "--opencl") == 0)
      dev = freenect2.openDefaultDevice(new libfreenect2::OpenCLPacketPipeline());
    else 
#endif
    if(strcmp(argv[i], "--opengl") == 0)
      dev = freenect2.openDefaultDevice(new libfreenect2::OpenGLPacketPipeline());
    else 
    if(strcmp(argv[i], "--cpu") == 0)
      dev = freenect2.openDefaultDevice(new libfreenect2::CpuPacketPipeline());
    else
    {
      std::cout << "Usage: Protonect [option]" << std::endl
        << "Packet processor type may be selected with these options:" << std::endl
        << "\t--opengl\tUse OpenGL to process depth packets" << std::endl
        << "\t--cpu\tUse CPU to process depth packets (slower)" << std::endl
#ifdef LIBFREENECT2_WITH_OPENCL_SUPPORT
        << "\t--opencl\tUse OpenCL to process depth packets" << std::endl
#else
        << std::endl << "This Protonect was not compiled with OpenCL support, so OpenCL depth processor is not available." << std::endl
#endif
        << "Default is OpenGL processor." << std::endl;
      exit(1);
    }
  }
  if(dev == 0)
    dev = freenect2.openDefaultDevice();
      

  if(dev == 0)
  {
    std::cout << "no device connected or failure opening the default one!" << std::endl;
    return -1;
  }

  signal(SIGINT,sigint_handler);
  protonect_shutdown = false;

  libfreenect2::SyncMultiFrameListener listener(libfreenect2::Frame::Color | libfreenect2::Frame::Ir | libfreenect2::Frame::Depth);
  libfreenect2::FrameMap frames;

  dev->setColorFrameListener(&listener);
  dev->setIrAndDepthFrameListener(&listener);
  dev->start();

  std::cout << "device serial: " << dev->getSerialNumber() << std::endl;
  std::cout << "device firmware: " << dev->getFirmwareVersion() << std::endl;

  while(!protonect_shutdown)
  {
    listener.waitForNewFrame(frames);
    libfreenect2::Frame *rgb = frames[libfreenect2::Frame::Color];
    libfreenect2::Frame *ir = frames[libfreenect2::Frame::Ir];
    libfreenect2::Frame *depth = frames[libfreenect2::Frame::Depth];

    cv::imshow("rgb", cv::Mat(rgb->height, rgb->width, CV_8UC3, rgb->data));
    cv::imshow("ir", cv::Mat(ir->height, ir->width, CV_32FC1, ir->data) / 20000.0f);
    cv::imshow("depth", cv::Mat(depth->height, depth->width, CV_32FC1, depth->data) / 4500.0f);

    int key = cv::waitKey(1);
    protonect_shutdown = protonect_shutdown || (key > 0 && ((key & 0xFF) == 27)); // shutdown on escape

    listener.release(frames);
    //libfreenect2::this_thread::sleep_for(libfreenect2::chrono::milliseconds(100));
  }

  // TODO: restarting ir stream doesn't work!
  // TODO: bad things will happen, if frame listeners are freed before dev->stop() :(
  dev->stop();
  dev->close();

  return 0;
}
