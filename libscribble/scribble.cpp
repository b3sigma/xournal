#include <iostream>
#include <memory>
#include <stdio.h>

#include "scribble.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_resize.h"
#include "stb_image_write.h"
#include "tiny_cnn/tiny_cnn.h"

// triple ugh
using namespace tiny_cnn;
using namespace tiny_cnn::activation;
using namespace tiny_cnn::layers;

using namespace std; //ugh

void sample1_convnet(const string& data_dir_path = "../../data");
void sample2_mlp();
void sample3_dae();
void sample4_dropout();
void sample5_unbalanced_training_data(const string& data_dir_path = "../../data");
void sample6_graph();
void recognize(const std::string& dictionary, const std::string& filename);

// rescale output to 0-100
template <typename Activation>
double rescale(double x) {
    Activation a;
    return 100.0 * (x - a.scale().first) / (a.scale().second - a.scale().first);
}

void convert_image(const std::string& imagefilename,
    double minv,
    double maxv,
    int w,
    int h,
    vec_t& data) {
	// load
	int input_w, input_h, comp;
	stbi_uc* input_pixels = stbi_load(imagefilename.c_str(), &input_w, &input_h, &comp, 1);
	if (!input_pixels) {
		// cannot open, or it's not an image
		cout << "stbi_load failed";
		return;
	}

	// resize
	std::vector<uint8_t> resized(w * h);
	uint8_t* resized_pixels = &(resized[0]);
	int input_stride_in_bytes = input_w;
	if (!stbir_resize_uint8(input_pixels, input_w, input_h, input_stride_in_bytes, resized_pixels, w, h, w, 1)) {
		cout << "stbir_resize_uint8 failed";
		stbi_image_free(input_pixels);
		return;
	}
	stbi_image_free(input_pixels);

    // mnist dataset is "white on black", so negate required
    std::transform(resized.begin(), resized.end(), std::back_inserter(data),
        [=](uint8_t c) { return (255 - c) * (maxv - minv) / 255.0 + minv; });
}

bool save_image(const std::string& imagefilename,
	const image<>& img
	)
{
	// no scaling, save at original size
	int stride_bytes = img.width();
	int ret = stbi_write_png(
		imagefilename.c_str(),
		img.width(),
		img.height(),
		1,
		&(img.at(0, 0)),
		stride_bytes);
	return (ret != 0);
}

void construct_net(network<sequential>& nn) {
    // connection table [Y.Lecun, 1998 Table.1]
#define O true
#define X false
    static const bool tbl[] = {
        O, X, X, X, O, O, O, X, X, O, O, O, O, X, O, O,
        O, O, X, X, X, O, O, O, X, X, O, O, O, O, X, O,
        O, O, O, X, X, X, O, O, O, X, X, O, X, O, O, O,
        X, O, O, O, X, X, O, O, O, O, X, X, O, X, O, O,
        X, X, O, O, O, X, X, O, O, O, O, X, O, O, X, O,
        X, X, X, O, O, O, X, X, O, O, O, O, X, O, O, O
    };
#undef O
#undef X

    // construct nets
    nn << convolutional_layer<tan_h>(32, 32, 5, 1, 6)  // C1, 1@32x32-in, 6@28x28-out
       << average_pooling_layer<tan_h>(28, 28, 6, 2)   // S2, 6@28x28-in, 6@14x14-out
       << convolutional_layer<tan_h>(14, 14, 5, 6, 16,
            connection_table(tbl, 6, 16))              // C3, 6@14x14-in, 16@10x10-in
       << average_pooling_layer<tan_h>(10, 10, 16, 2)  // S4, 16@10x10-in, 16@5x5-out
       << convolutional_layer<tan_h>(5, 5, 5, 16, 120) // C5, 16@5x5-in, 120@1x1-out
       << fully_connected_layer<tan_h>(120, 10);       // F6, 120-in, 10-out
}

class ScribbleData {
  public:
    network<sequential> nn;
};

extern "C" {
    bool initialize(char* weights_filename, void** out_scribble_handle) {
        std::unique_ptr<ScribbleData> scribble(new ScribbleData);

        construct_net(scribble->nn);

        // load nets
        string default_weights("LeNet-weights");
        ifstream ifs((weights_filename != NULL) ? weights_filename : default_weights.c_str());
        if(!ifs) { return false; }
        ifs >> scribble->nn;

        *out_scribble_handle = scribble.release();

        return true;
    }
} //extern C

extern "C" {
    void cleanup(void* scribble_handle) {
        ScribbleData* scribble = (ScribbleData*)scribble_handle;
        delete scribble;
    }
} //extern C

bool convert_memory_image(unsigned char* data,
        int x_size, int y_size, int pitch_bytes, int y_stride_bytes,
        vec_t& output) {

    if(x_size != 32 || y_size != 32 || pitch_bytes != 3
            || (y_stride_bytes < (x_size * pitch_bytes))) {
        printf("Only supporting 32x32 of 8 bit 3 channel rgb right now\n");
        return false;
    }

    // output is currently 32x32 and single floating point channel (-1.0,1.0)
    double min_val = -1.0;
    double max_val = 1.0;
    double range = max_val - min_val;

    // printf("so the bytes of red were (x:%d y:%d pit:%d str:%d)\n",
    //     x_size, y_size, pitch_bytes, y_stride_bytes);
    output.resize(0);
    output.reserve(x_size * y_size);
    for(int y = 0; y < y_size; y++) {
        for(int x = 0; x < x_size; x++) {
            unsigned char* pixel_channels = (unsigned char*)(data + (y_stride_bytes * y) + (pitch_bytes * x));
            double red    = (double)(255 - pixel_channels[0]) / 255.0;
            double green  = (double)(255 - pixel_channels[1]) / 255.0;
            double blue   = (double)(255 - pixel_channels[2]) / 255.0;
            // printf("%d ", pixel_channels[0]);
            float result = ((red * range) + min_val);
            output.push_back(result);
        }
        // printf("\n");
    }
    return true;
}

extern "C" {
    bool memory_recognize(void* scribble_handle, unsigned char* image_data,
            int x_size, int y_size, int pitch_bytes, int y_stride_bytes,
            int* out_what_number) {
        ScribbleData* scribble = (ScribbleData*)scribble_handle;

        vec_t data;
        convert_memory_image(image_data, x_size, y_size,
            pitch_bytes, y_stride_bytes, data);
        printf("yep this is terrible and data had %d, with "
            "first(%f) third(%f) and last(%f)\n", 
            (int)data.size(), data[0], data[2], data.back());

        auto input_debug = vec2image<unsigned char>(data, {32, 32, 1});
        auto filename = "input_debug.png";
        if(!save_image(filename, input_debug)) {
            cout << "failed to save " << filename << endl;
        } else {
            cout << "apparently did save " << filename << endl;
        }

        // recognize
        auto res = scribble->nn.predict(data);
        vector<pair<double, int> > scores;

        // sort & print top-3
        for (int i = 0; i < 10; i++)
            scores.emplace_back(rescale<tan_h>(res[i]), i);

        sort(scores.begin(), scores.end(), greater<pair<double, int>>());

        for (int i = 0; i < 3; i++)
            cout << scores[i].second << "," << scores[i].first << endl;

        *out_what_number = scores[0].second;

        

        // save outputs of each layer
        for (size_t i = 0; i < scribble->nn.depth(); i++) {
            auto out_img = scribble->nn[i]->output_to_image();
            auto filename = "layer_" + std::to_string(i) + ".png";
            if (!save_image(filename, out_img)) {
                cout << "failed to save " << filename << endl;
            } else {
                cout << "apparently did save " << filename << endl;
            }
        }
        // save filter shape of first convolutional layer
        {
            auto weight = scribble->nn.at<convolutional_layer<tan_h>>(0).weight_to_image();
            auto filename = "weights.png";
            if (!save_image(filename, weight)) {
                cout << "failed to save " << filename << endl;
            } else {
                cout << "apparently did save " << filename << endl;
            }
        }


        return true;
    }
} // extern C

void recognize(const std::string& dictionary, const std::string& filename) {
    network<sequential> nn;

    construct_net(nn);

    // load nets
    ifstream ifs(dictionary.c_str());
    ifs >> nn;

    // convert imagefile to vec_t
    vec_t data;
    convert_image(filename, -1.0, 1.0, 32, 32, data);

    // recognize
    auto res = nn.predict(data);
    vector<pair<double, int> > scores;

    // sort & print top-3
    for (int i = 0; i < 10; i++)
        scores.emplace_back(rescale<tan_h>(res[i]), i);

    sort(scores.begin(), scores.end(), greater<pair<double, int>>());

    for (int i = 0; i < 3; i++)
        cout << scores[i].second << "," << scores[i].first << endl;

    // save outputs of each layer
    for (size_t i = 0; i < nn.depth(); i++) {
        auto out_img = nn[i]->output_to_image();
		auto filename = "layer_" + std::to_string(i) + ".png";
		if (!save_image(filename, out_img)) {
			cout << "failed to save " << filename << endl;
		}
    }
    // save filter shape of first convolutional layer
	{
	    auto weight = nn.at<convolutional_layer<tan_h>>(0).weight_to_image();
		auto filename = "weights.png";
		if (!save_image(filename, weight)) {
			cout << "failed to save " << filename << endl;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// learning convolutional neural networks (LeNet-5 like architecture)
void sample1_convnet(const string& data_dir_path) {
    // construct LeNet-5 architecture
    network<sequential> nn;
    adagrad optimizer;

    // connection table [Y.Lecun, 1998 Table.1]
#define O true
#define X false
    static const bool connection [] = {
        O, X, X, X, O, O, O, X, X, O, O, O, O, X, O, O,
        O, O, X, X, X, O, O, O, X, X, O, O, O, O, X, O,
        O, O, O, X, X, X, O, O, O, X, X, O, X, O, O, O,
        X, O, O, O, X, X, O, O, O, O, X, X, O, X, O, O,
        X, X, O, O, O, X, X, O, O, O, O, X, O, O, X, O,
        X, X, X, O, O, O, X, X, O, O, O, O, X, O, O, O
    };
#undef O
#undef X

    nn << convolutional_layer<tan_h>(32, 32, 5, 1, 6) // 32x32 in, 5x5 kernel, 1-6 fmaps conv
       << average_pooling_layer<tan_h>(28, 28, 6, 2) // 28x28 in, 6 fmaps, 2x2 subsampling
       << convolutional_layer<tan_h>(14, 14, 5, 6, 16,
                                     connection_table(connection, 6, 16)) // with connection-table
       << average_pooling_layer<tan_h>(10, 10, 16, 2)
       << convolutional_layer<tan_h>(5, 5, 5, 16, 120)
       << fully_connected_layer<tan_h>(120, 10);

    std::cout << "load models..." << std::endl;

    // load MNIST dataset
    std::vector<label_t> train_labels, test_labels;
    std::vector<vec_t> train_images, test_images;

    parse_mnist_labels(data_dir_path + "/train-labels.idx1-ubyte", &train_labels);
    parse_mnist_images(data_dir_path + "/train-images.idx3-ubyte", &train_images, -1.0, 1.0, 2, 2);
    parse_mnist_labels(data_dir_path + "/t10k-labels.idx1-ubyte", &test_labels);
    parse_mnist_images(data_dir_path + "/t10k-images.idx3-ubyte", &test_images, -1.0, 1.0, 2, 2);

    std::cout << "start learning" << std::endl;

    progress_display disp(train_images.size());
    timer t;
    int minibatch_size = 10;

    optimizer.alpha *= std::sqrt(minibatch_size);

    // create callback
    auto on_enumerate_epoch = [&](){
        std::cout << t.elapsed() << "s elapsed." << std::endl;

        tiny_cnn::result res = nn.test(test_images, test_labels);

        std::cout << res.num_success << "/" << res.num_total << std::endl;

        disp.restart(train_images.size());
        t.restart();
    };

    auto on_enumerate_minibatch = [&](){ 
        disp += minibatch_size; 
    };
    
    // training
    nn.train<mse>(optimizer, train_images, train_labels, minibatch_size, 20, on_enumerate_minibatch, on_enumerate_epoch);

    std::cout << "end training." << std::endl;

    // test and show results
    nn.test(test_images, test_labels).print_detail(std::cout);

    // save networks
    std::ofstream ofs("LeNet-weights");
    ofs << nn;
}


///////////////////////////////////////////////////////////////////////////////
// learning 3-Layer Networks
void sample2_mlp()
{
    const cnn_size_t num_hidden_units = 500;

#if defined(_MSC_VER) && _MSC_VER < 1800
    // initializer-list is not supported
    int num_units[] = { 28 * 28, num_hidden_units, 10 };
    auto nn = make_mlp<tan_h>(num_units, num_units + 3);
#else
    auto nn = make_mlp<tan_h>({ 28 * 28, num_hidden_units, 10 });
#endif
    gradient_descent optimizer;

    // load MNIST dataset
    std::vector<label_t> train_labels, test_labels;
    std::vector<vec_t> train_images, test_images;

    parse_mnist_labels("../../data/train-labels.idx1-ubyte", &train_labels);
    parse_mnist_images("../../data/train-images.idx3-ubyte", &train_images, -1.0, 1.0, 0, 0);
    parse_mnist_labels("../../data/t10k-labels.idx1-ubyte", &test_labels);
    parse_mnist_images("../../data/t10k-images.idx3-ubyte", &test_images, -1.0, 1.0, 0, 0);

    optimizer.alpha = 0.001;
    
    progress_display disp(train_images.size());
    timer t;

    // create callback
    auto on_enumerate_epoch = [&](){
        std::cout << t.elapsed() << "s elapsed." << std::endl;

        tiny_cnn::result res = nn.test(test_images, test_labels);

        std::cout << optimizer.alpha << "," << res.num_success << "/" << res.num_total << std::endl;

        optimizer.alpha *= 0.85; // decay learning rate
        optimizer.alpha = std::max((tiny_cnn::float_t)0.00001, optimizer.alpha);

        disp.restart(train_images.size());
        t.restart();
    };

    auto on_enumerate_data = [&](){ 
        ++disp; 
    };  

    nn.train<mse>(optimizer, train_images, train_labels, 1, 20, on_enumerate_data, on_enumerate_epoch);
}

///////////////////////////////////////////////////////////////////////////////
// denoising auto-encoder
void sample3_dae()
{
#if defined(_MSC_VER) && _MSC_VER < 1800
    // initializer-list is not supported
    int num_units[] = { 100, 400, 100 };
    auto nn = make_mlp<tan_h>(num_units, num_units + 3);
#else
    auto nn = make_mlp<tan_h>({ 100, 400, 100 });
#endif

    std::vector<vec_t> train_data_original;

    // load train-data

    std::vector<vec_t> train_data_corrupted(train_data_original);

    for (auto& d : train_data_corrupted) {
        d = corrupt(move(d), 0.1, 0.0); // corrupt 10% data
    }

    gradient_descent optimizer;

    // learning 100-400-100 denoising auto-encoder
    nn.train<mse>(optimizer, train_data_corrupted, train_data_original);
}

///////////////////////////////////////////////////////////////////////////////
// dropout-learning

void sample4_dropout()
{
    typedef network<sequential> Network;
    Network nn;
    cnn_size_t input_dim    = 28*28;
    cnn_size_t hidden_units = 800;
    cnn_size_t output_dim   = 10;
    gradient_descent optimizer;

    fully_connected_layer<tan_h> f1(input_dim, hidden_units);
    dropout_layer dropout(hidden_units, 0.5);
    fully_connected_layer<tan_h> f2(hidden_units, output_dim);
    nn << f1 << dropout << f2;

    optimizer.alpha = 0.003; // TODO: not optimized
    optimizer.lambda = 0.0;

    // load MNIST dataset
    std::vector<label_t> train_labels, test_labels;
    std::vector<vec_t> train_images, test_images;

    parse_mnist_labels("../../data/train-labels.idx1-ubyte", &train_labels);
    parse_mnist_images("../../data/train-images.idx3-ubyte", &train_images, -1.0, 1.0, 0, 0);
    parse_mnist_labels("../../data/t10k-labels.idx1-ubyte", &test_labels);
    parse_mnist_images("../../data/t10k-images.idx3-ubyte", &test_images, -1.0, 1.0, 0, 0);

    // load train-data, label_data
    progress_display disp(train_images.size());
    timer t;

    // create callback
    auto on_enumerate_epoch = [&](){
        std::cout << t.elapsed() << "s elapsed." << std::endl;
  
        dropout.set_context(net_phase::test);
        tiny_cnn::result res = nn.test(test_images, test_labels);
        dropout.set_context(net_phase::train);


        std::cout << optimizer.alpha << "," << res.num_success << "/" << res.num_total << std::endl;

        optimizer.alpha *= 0.99; // decay learning rate
        optimizer.alpha = std::max((tiny_cnn::float_t)0.00001, optimizer.alpha);

        disp.restart(train_images.size());
        t.restart();
    };

    auto on_enumerate_data = [&](){
        ++disp;
    };

    nn.train<mse>(optimizer, train_images, train_labels, 1, 100, on_enumerate_data, on_enumerate_epoch);

    // change context to enable all hidden-units
    //f1.set_context(dropout::test_phase);
    //std::cout << res.num_success << "/" << res.num_total << std::endl;
}

#include "tiny_cnn/util/target_cost.h"

///////////////////////////////////////////////////////////////////////////////
// learning unbalanced training data

void sample5_unbalanced_training_data(const string& data_dir_path)
{
    const cnn_size_t num_hidden_units = 20; // keep the network relatively simple
    auto nn_standard = make_mlp<tan_h>({ 28 * 28, num_hidden_units, 10 });
    auto nn_balanced = make_mlp<tan_h>({ 28 * 28, num_hidden_units, 10 });
    gradient_descent optimizer;

    // load MNIST dataset
    std::vector<label_t> train_labels, test_labels;
    std::vector<vec_t>   train_images, test_images;

    parse_mnist_labels(data_dir_path + "/train-labels.idx1-ubyte", &train_labels);
    parse_mnist_images(data_dir_path + "/train-images.idx3-ubyte", &train_images, -1.0, 1.0, 0, 0);
    parse_mnist_labels(data_dir_path + "/t10k-labels.idx1-ubyte", &test_labels);
    parse_mnist_images(data_dir_path + "/t10k-images.idx3-ubyte", &test_images, -1.0, 1.0, 0, 0);

    { // create an unbalanced training set
        std::vector<label_t> train_labels_unbalanced;
        std::vector<vec_t>   train_images_unbalanced;
        train_labels_unbalanced.reserve(train_labels.size());
        train_images_unbalanced.reserve(train_images.size());

        for (size_t i = 0, end = train_labels.size(); i < end; ++i) {
            const label_t label = train_labels[i];

            // drop most 0s, 1s, 2s, 3s, and 4s
            const bool keep_sample = label >= 5 || bernoulli(0.005);

            if (keep_sample) {
                train_labels_unbalanced.push_back(label);
                train_images_unbalanced.push_back(train_images[i]);
            }
        }

        // keep the newly created unbalanced training set
        std::swap(train_labels, train_labels_unbalanced);
        std::swap(train_images, train_images_unbalanced);
    }

    optimizer.alpha = 0.001;

    progress_display disp(train_images.size());
    timer t;

    const int minibatch_size = 10;

    auto nn = &nn_standard; // the network referred to by the callbacks

    // create callbacks - as usual
    auto on_enumerate_epoch = [&](){
        std::cout << t.elapsed() << "s elapsed." << std::endl;

        tiny_cnn::result res = nn->test(test_images, test_labels);

        std::cout << optimizer.alpha << "," << res.num_success << "/" << res.num_total << std::endl;

        optimizer.alpha *= 0.85; // decay learning rate
        optimizer.alpha = std::max((tiny_cnn::float_t)0.00001, optimizer.alpha);

        disp.restart(train_images.size());
        t.restart();
    };

    auto on_enumerate_data = [&](){
        disp += minibatch_size;
    };

    // first train the standard network (default cost - equal for each sample)
    // - note that it does not learn the classes 0-4
    nn_standard.train<mse>(optimizer, train_images, train_labels, minibatch_size, 20, on_enumerate_data, on_enumerate_epoch, true, CNN_TASK_SIZE);

    // then train another network, now with explicitly supplied target costs (aim: a more balanced predictor)
    // - note that it can learn the classes 0-4 (at least somehow)
    nn = &nn_balanced;
    optimizer = gradient_descent();
    const auto target_cost = create_balanced_target_cost(train_labels, 0.8);
    nn_balanced.train<mse>(optimizer, train_images, train_labels, minibatch_size, 20, on_enumerate_data, on_enumerate_epoch, true, CNN_TASK_SIZE, target_cost);

    // test and show results
    std::cout << std::endl << "Standard training (implicitly assumed equal cost for every sample):" << std::endl;
    nn_standard.test(test_images, test_labels).print_detail(std::cout);

    std::cout << std::endl << "Balanced training (explicitly supplied target costs):" << std::endl;
    nn_balanced.test(test_images, test_labels).print_detail(std::cout);
}

void sample6_graph()
{
    // declare node
    auto in1 = std::make_shared<input_layer>(shape3d(3, 1, 1));
    auto in2 = std::make_shared<input_layer>(shape3d(3, 1, 1));
    auto added = std::make_shared<add>(2, 3);
    auto out = std::make_shared<linear_layer<relu>>(3);

    // connect
    (in1, in2) << added;
    added << out;

    network<graph> net;
    construct_graph(net, { in1, in2 }, { out });

    auto res = net.predict({ { 2,4,3 },{ -1,2,-5 } })[0];

    // relu({2,4,3} + {-1,2,-5}) = {1,6,0}
    std::cout << res[0] << "," << res[1] << "," << res[2] << std::endl;
}

/*
    Copyright (c) 2013, Taiga Nomi
    All rights reserved.
    
    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
    * Neither the name of the <organization> nor the
    names of its contributors may be used to endorse or promote products
    derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY 
    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY 
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND 
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
