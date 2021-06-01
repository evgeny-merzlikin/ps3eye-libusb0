#ifndef PS3EYECAM_H
#define PS3EYECAM_H


#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include <memory>

// Get rid of annoying zero length structure warnings from libusb.h in MSVC

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4200)
#endif

#include "lusb0_usb.h"

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#ifndef __STDC_CONSTANT_MACROS
#  define __STDC_CONSTANT_MACROS
#endif

#include <stdint.h>

#if defined(DEBUG)
#define debug(...) fprintf(stdout, __VA_ARGS__)
#else
#define debug(...) 
#endif


namespace ps3eye {

class PS3EYECam
{
public:
	enum class EOutputFormat
	{
		Bayer,					// Output in Bayer. Destination buffer must be width * height bytes
		BGR,					// Output in BGR. Destination buffer must be width * height * 3 bytes
		RGB						// Output in RGB. Destination buffer must be width * height * 3 bytes
	};

	typedef std::shared_ptr<PS3EYECam> PS3EYERef;

	static const uint16_t VENDOR_ID;
	static const uint16_t PRODUCT_ID;

	PS3EYECam(struct usb_device *device);
	~PS3EYECam();

	bool init(uint32_t width = 0, uint32_t height = 0, uint16_t desiredFrameRate = 30, EOutputFormat outputFormat = EOutputFormat::RGB);
	void start();
	void stop();

	// Controls

	bool getAutogain() { 
		return (sccb_reg_read(0x13) == 0xf7) ? true : false; 
	}
	void setAutogain(bool val) {
	    if (val) {
			sccb_reg_write(0x13, 0xf7); //AGC,AEC,AWB ON
			sccb_reg_write(0x64, sccb_reg_read(0x64)|0x03);
	    } else {
			sccb_reg_write(0x13, 0xf0); //AGC,AEC,AWB OFF
			sccb_reg_write(0x64, sccb_reg_read(0x64)&0xFC);
	    }
	}

	bool getAutoWhiteBalance() { 
		return (sccb_reg_read(0x63) == 0xe0) ? true : false; 
	}
	void setAutoWhiteBalance(bool val) {
	    if (val) {
			sccb_reg_write(0x63, 0xe0); //AWB ON
	    }else{
			sccb_reg_write(0x63, 0xAA); //AWB OFF
	    }
	}
	bool getAutoExposure() { 
		return (sccb_reg_read(0x13) | 0x01) ? true : false; 
	}
	void setAutoExposure(bool val) {
	    if (val) {
			sccb_reg_write(0x13, sccb_reg_read(0x13) | 0x01); // AEC on
	    }else{
			sccb_reg_write(0x13, sccb_reg_read(0x13) & ~0x01);// AEC off
	    }
	}
	uint8_t getGain() { 
		return sccb_reg_read(0x00); 
	}
	void setGain(uint8_t val) {
	    switch(val & 0x30){
		case 0x00:
		    val &=0x0F;
		    break;
		case 0x10:
		    val &=0x0F;
		    val |=0x30;
		    break;
		case 0x20:
		    val &=0x0F;
		    val |=0x70;
		    break;
		case 0x30:
		    val &=0x0F;
		    val |=0xF0;
		    break;
	    }
	    sccb_reg_write(0x00, val);
	}
	uint8_t getExposure()  {
		uint8_t val;
		uint8_t hi = sccb_reg_read(0x08);
		uint8_t lo = sccb_reg_read(0x10);
		val = (hi << 8 | lo) >> 1;
		return val;
	}
	void setExposure(uint8_t val) {
	    sccb_reg_write(0x08, val>>7);
    	sccb_reg_write(0x10, val<<1);
	}
	uint8_t getSharpness() { 
		if (frame_width == 320)
			return sccb_reg_read(0x8E); 
		return sccb_reg_read(0x91);
	}
	void setSharpness(uint8_t val) {
	    sccb_reg_write(0x91, val); //vga noise
    	sccb_reg_write(0x8E, val); //qvga noise
	}
	uint8_t getContrast() { return sccb_reg_read(0x9C); }
	void setContrast(uint8_t val) { sccb_reg_write(0x9C, val); }

	uint8_t getBrightness() { return sccb_reg_read(0x9B); }
	void setBrightness(uint8_t val) { sccb_reg_write(0x9B, val); }

	uint8_t getHue() { return sccb_reg_read(0x01); }
	void setHue(uint8_t val) { sccb_reg_write(0x01, val); }

	uint8_t getRedBalance() { return sccb_reg_read(0x43); }
	void setRedBalance(uint8_t val) { sccb_reg_write(0x43, val); }

	uint8_t getBlueBalance() { return sccb_reg_read(0x42); }
	void setBlueBalance(uint8_t val) { sccb_reg_write(0x42, val); }

	uint8_t getGreenBalance() { return sccb_reg_read(0x44); }
	void setGreenBalance(uint8_t val) { sccb_reg_write(0x44, val); }

    bool getFlipH() { 
		uint8_t val = sccb_reg_read(0x0c);
        val &= ~0xc0;
        return (val |= 0x40) != 0;
	}
    bool getFlipV() { 
		uint8_t val = sccb_reg_read(0x0c);
        val &= ~0xc0;
        return (val |= 0x80) != 0;
	}
	void setFlip(bool horizontal = false, bool vertical = false) {
		uint8_t val = sccb_reg_read(0x0c);
        val &= ~0xc0;
        if (!horizontal) val |= 0x40;
        if (!vertical) val |= 0x80;
        sccb_reg_write(0x0c, val);
	}
    

    bool isStreaming() const { return is_streaming; }
    bool isInitialized() const { return device_ != NULL && handle_ != NULL && usb_buf != NULL; }

	//bool getUSBPortPath(char *out_identifier, size_t max_identifier_length) const;
	
	// Get a frame from the camera. Notes:
	// - If there is no frame available, this function will block until one is
	// - The output buffer must be sized correctly, depending out the output format. See EOutputFormat.
	void getFrame(uint8_t* frame);

	uint32_t getWidth() const { return frame_width; }
	uint32_t getHeight() const { return frame_height; }
	uint16_t getFrameRate() const { return frame_rate; }
	bool setFrameRate(uint8_t val) {
		if (is_streaming) return false;
		frame_rate = ov534_set_frame_rate(val, true);
		return true;
	}
	uint32_t getRowBytes() const { return frame_width * getOutputBytesPerPixel(); }
	uint32_t getOutputBytesPerPixel() const;

	//
	static const std::vector<PS3EYERef>& getDevices( bool forceRefresh = false );
	void ov534_reg_write(uint16_t reg, uint8_t val);
	uint8_t ov534_reg_read(uint16_t reg);
	void sccb_reg_write(uint8_t reg, uint8_t val);
	uint8_t sccb_reg_read(uint16_t reg);

private:
	PS3EYECam(const PS3EYECam&);
    void operator=(const PS3EYECam&);

	void release();

	// usb ops
	uint16_t ov534_set_frame_rate(uint16_t frame_rate, bool dry_run = false);
	void ov534_set_led(int status);

	int sccb_check_status();

	void reg_w_array(const uint8_t (*data)[2], int len);
	void sccb_w_array(const uint8_t (*data)[2], int len);

    bool is_streaming;

	std::shared_ptr<class USBMgr> mgrPtr;

	static bool devicesEnumerated;
    static std::vector<PS3EYERef> devices;

	uint32_t frame_width;
	uint32_t frame_height;
	uint16_t frame_rate;
	EOutputFormat frame_output_format;

	//usb stuff
	struct usb_device *device_;
	struct usb_dev_handle *handle_;
	uint8_t *usb_buf;

	std::shared_ptr<class URBDesc> urb;

	bool open_usb();
	void close_usb();

};

} // namespace


#endif
