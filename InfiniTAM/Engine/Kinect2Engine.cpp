// Copyright 2014-2015 Isis Innovation Limited and the authors of InfiniTAM

#include "Kinect2Engine.h"

#include "../Utils/FileUtils.h"

#include <stdio.h>

#ifndef COMPILE_WITHOUT_Kinect2
#include <Kinect.h>

#pragma comment(lib, "kinect20.lib")

using namespace InfiniTAM::Engine;

// Safe release for interfaces
template<class Interface>
inline void SafeRelease(Interface *& pInterfaceToRelease)
{
	if (pInterfaceToRelease != NULL)
	{
		pInterfaceToRelease->Release();
		pInterfaceToRelease = NULL;
	}
}

class Kinect2Engine::PrivateData {
	public:
	PrivateData(void) {}

	IKinectSensor* kinectSensor;
	IDepthFrameReader* depthFrameReader;
	IColorFrameReader* colorFrameReader;
};

Kinect2Engine::Kinect2Engine(const char *calibFilename) : ImageSourceEngine(calibFilename)
{
	imageSize_d = Vector2i(512, 424);
	imageSize_rgb = Vector2i(640, 480);
	
	data = new PrivateData();

	colorAvailable = true;

	HRESULT hr;

	depthAvailable = true;

	hr = GetDefaultKinectSensor(&data->kinectSensor);
	if (FAILED(hr))
	{
		depthAvailable = false;
		printf("Kinect2: Failed to initialise depth camera\n");
		return;
	}

	if (data->kinectSensor)
	{
		IDepthFrameSource* pDepthFrameSource = NULL;
		IColorFrameSource* pColorFrameSource = NULL;

		hr = data->kinectSensor->Open();

		if (SUCCEEDED(hr))
			hr = data->kinectSensor->get_DepthFrameSource(&pDepthFrameSource);

		if (SUCCEEDED(hr))
			hr = pDepthFrameSource->OpenReader(&data->depthFrameReader);

		if (SUCCEEDED(hr))
			hr = data->kinectSensor->get_ColorFrameSource(&pColorFrameSource);

		if (SUCCEEDED(hr))
			hr = pColorFrameSource->OpenReader(&data->colorFrameReader);

		SafeRelease(pDepthFrameSource);
		SafeRelease(pColorFrameSource);
	}

	if (!data->kinectSensor || FAILED(hr))
	{
		depthAvailable = false;
		printf("Kinect2: No ready Kinect 2 sensor found\n");
		return;
	}
}

Kinect2Engine::~Kinect2Engine()
{
	SafeRelease(data->depthFrameReader);
	SafeRelease(data->colorFrameReader);

	if (data->kinectSensor) data->kinectSensor->Close();

	SafeRelease(data->kinectSensor);
}

void Kinect2Engine::getImages(ITMUChar4Image *rgbImage, ITMShortImage *rawDepthImage)
{
	Vector4u *rgb = rgbImage->GetData(MEMORYDEVICE_CPU);
	BYTE* bigImg = new BYTE(4 * 1920 * 1080);
	if (colorAvailable)
	{
		IColorFrame* pColorFrame = NULL;
		UCHAR *pBuffer = NULL;
		UINT nBufferSize = 0;

		HRESULT hr = data->colorFrameReader->AcquireLatestFrame(&pColorFrame);
		IFrameDescription *frameDescription;
		pColorFrame->get_FrameDescription(&frameDescription);
		int width, height;
		frameDescription->get_Width(&width);
		frameDescription->get_Height(&height);
		std::cout << width << " " << height << std::endl;

		if (SUCCEEDED(hr))
		{
			hr = pColorFrame->CopyConvertedFrameDataToArray(4*1920*1080, bigImg, ColorImageFormat_Rgba);
			for (size_t r = 0; r < imageSize_rgb.y; ++r) {
				for (size_t c = 0; c < imageSize_rgb.x; ++c) {
					size_t bigR = floor(r * 2.25), bigC = c * 3;
					size_t b = 4*(1920 * bigR + bigC);
					Vector4u out = Vector4u(
						bigImg[b], bigImg[b + 1], bigImg[b + 2], bigImg[b + 3]);
					rgb[r*imageSize_rgb.x + c] = out;
				}
			}
			if (!SUCCEEDED(hr)) {
				throw 1;
			}
		}

		SafeRelease(pColorFrame);
	}
	//else memset(rgb, 0, rgbImage->dataSize * sizeof(Vector4u));

	short *depth = rawDepthImage->GetData(MEMORYDEVICE_CPU);
	if (depthAvailable)
	{
		IDepthFrame* pDepthFrame = NULL;
		UINT16 *pBuffer = NULL;
		UINT nBufferSize = 0;

		HRESULT hr = data->depthFrameReader->AcquireLatestFrame(&pDepthFrame);

		if (SUCCEEDED(hr))
		{
			if (SUCCEEDED(hr))
				hr = pDepthFrame->AccessUnderlyingBuffer(&nBufferSize, &pBuffer);

			if (SUCCEEDED(hr))
			{
				for (int i = 0; i < imageSize_d.x * imageSize_d.y; i++)
				{
					ushort depthPix = pBuffer[i];
					depth[i] = depthPix;
					//depth[i] = depthPix == 0 ? -1.0f : (float)depthPix / 1000.0f;
				}
			}
		}

		SafeRelease(pDepthFrame);
	}
	//else memset(depth, 0, out->depth->dataSize * sizeof(short));

	//out->inputImageType = ITMView::InfiniTAM_FLOAT_DEPTH_IMAGE;

	return /*true*/;
}

bool Kinect2Engine::hasMoreImages(void) { return true; }
Vector2i Kinect2Engine::getDepthImageSize(void) { return imageSize_d; }
Vector2i Kinect2Engine::getRGBImageSize(void) { return imageSize_rgb; }

#else

using namespace InfiniTAM::Engine;

Kinect2Engine::Kinect2Engine(const char *calibFilename) : ImageSourceEngine(calibFilename)
{
	printf("compiled without Kinect 2 support\n");
}
Kinect2Engine::~Kinect2Engine()
{}
void Kinect2Engine::getImages(ITMUChar4Image *rgbImage, ITMShortImage *rawDepthImage)
{ return; }
bool Kinect2Engine::hasMoreImages(void)
{ return false; }
Vector2i Kinect2Engine::getDepthImageSize(void)
{ return Vector2i(0,0); }
Vector2i Kinect2Engine::getRGBImageSize(void)
{ return Vector2i(0,0); }

#endif

