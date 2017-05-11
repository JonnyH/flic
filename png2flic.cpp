#include "flic.h"
#include "lodepng/lodepng.h"

#include <boost/program_options.hpp>
#include <cstdio>
#include <iostream>

namespace po = boost::program_options;

static bool read_frame(const std::string &file_name, const flic::Header &header,
                       flic::Frame &out_frame)
{
	lodepng::State png_state;
	std::vector<unsigned char> png_data;
	std::vector<unsigned char> pixels;
	unsigned height, width;

	png_state.info_raw.colortype = LCT_PALETTE;
	png_state.info_raw.bitdepth = 8;

	unsigned error = lodepng::load_file(png_data, file_name.c_str());
	if (error)
	{
		std::cerr << "Failed to read file - lodepng error " << error << " \""
		          << lodepng_error_text(error) << "\"\n";
		return false;
	}

	error = lodepng::decode(pixels, width, height, png_state, png_data);
	if (error)
	{
		std::cerr << "Failed to decode file - lodepng erro " << error << " \""
		          << lodepng_error_text(error) << "\"\n";
		return false;
	}

	if (png_state.info_raw.palettesize > 256)
	{
		std::cerr << "Invalid palette size " << png_state.info_raw.palettesize << "\n";
		return false;
	}

	if (height != header.height)
	{
		std::cerr << "image height doesn't match video\n";
		return false;
	}
	if (width != header.width)
	{
		std::cerr << "image width doesn't match video\n";
		return false;
	}
	if (width != out_frame.rowstride)
	{
		std::cerr << "Frame row stride doesn't match width\n";
		return false;
	}
	if (pixels.size() != height * width)
	{
		std::cerr << "Unexpected number of pixels returned\n";
		return false;
	}
	std::cout << "writing " << png_state.info_png.color.palettesize << " palette entries\n";
	size_t entry = 0;
	for (; entry < png_state.info_png.color.palettesize; entry++)
	{
		// the palette is RGBA at 8bpp
		const uint8_t *pos = png_state.info_png.color.palette + (4 * entry);
		uint8_t r = *pos++;
		uint8_t g = *pos++;
		uint8_t b = *pos++;
		uint8_t a = *pos++;
		out_frame.colormap[entry] = flic::Color(r, g, b);
	}
	for (; entry < 256; entry++)
	{
		out_frame.colormap[entry] = flic::Color{0, 0, 0};
	}
	memcpy(out_frame.pixels, pixels.data(), pixels.size());
	return true;
}

int main(int argc, char **argv)
{
	int height = 0, width = 0, speed = 0;

	std::string output;
	std::vector<std::string> input_files;

	po::options_description opts("Allowed options");
	opts.add_options()("help", "show help message (this)")(
	    "output,o", po::value<std::string>(&output),
	    "set output file")("height,h", po::value<int>(&height), "set video height")(
	    "width,w", po::value<int>(&width), "set video width")("speed,s", po::value<int>(&speed),
	                                                          "set video speed");

	po::options_description hidden_opts("Hidden options");
	hidden_opts.add_options()("input-file", po::value<std::vector<std::string>>(&input_files),
	                          "input file");

	po::positional_options_description pos_opts;
	pos_opts.add("input-file", -1);

	po::options_description all_opts;
	all_opts.add(opts).add(hidden_opts);

	po::variables_map vm;
	po::store(po::command_line_parser(argc, argv).options(all_opts).positional(pos_opts).run(), vm);

	po::notify(vm);

	if (vm.count("help"))
	{
		std::cout << "Usage: png2flic [options] input1.png, input2.png ...\n";
		std::cout << opts;
		return EXIT_SUCCESS;
	}

	if (output.empty())
	{
		std::cerr << "must specify output file\n";
		return EXIT_FAILURE;
	}

	if (height <= 0)
	{
		std::cerr << "Must specify valid height\n";
		return EXIT_FAILURE;
	}

	if (width <= 0)
	{
		std::cerr << "Must specify valid width\n";
		return EXIT_FAILURE;
	}

	if (speed <= 0)
	{
		std::cerr << "Must specify valid speed\n";
		return EXIT_FAILURE;
	}

	if (input_files.empty())
	{
		std::cerr << "Must specify at least one input file\n";
		return EXIT_FAILURE;
	}

	std::cout << "output: " << output << "\n";
	std::cout << "height: " << height << "\n";
	std::cout << "width: " << width << "\n";

	FILE *output_file = fopen(output.c_str(), "w");
	if (!output_file)
	{
		std::cerr << "Failed to open output file \"" << output_file << "\"\n";
		return EXIT_FAILURE;
	}
	flic::StdioFileInterface output_file_flic(output_file);

	flic::Header header;
	header.frames = 0; // frames is filled in at the end of the encoder anyway?
	header.width = width;
	header.height = height;
	header.speed = speed;
	int ret = EXIT_SUCCESS;

	{
		flic::Encoder encoder(&output_file_flic);
		encoder.writeHeader(header);

		flic::Frame frame;
		frame.rowstride = width;
		frame.pixels = new uint8_t[width * height];

		for (auto &f : input_files)
		{
			std::cout << "Reading " << f << "\n";
			if (!read_frame(f, header, frame))
			{
				std::cerr << "Failed to read input file \"" << f << "\"\n";
				ret = EXIT_FAILURE;
				break;
			}
			encoder.writeFrame(frame);
		}
		delete[] frame.pixels;
	}

	fclose(output_file);

	return ret;
}
