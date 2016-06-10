#include "IMCHandler.h"
#include <folly/futures/Future.h>

#include <sstream>
#include <string>
#include <vector>
#include <unistd.h>

#include <jpeglib.h>
#include <gflags/gflags.h>

DEFINE_string(imc_network, "configs/imc.prototxt",
              "Network config for imc (default: config/imc.prototxt");

DEFINE_string(imc_weights, "../models/imc.caffemodel",
              "Weight config for imc (default: models/imc.caffemodel");

using caffe::Blob;
using caffe::Caffe;
using caffe::Net;

using namespace folly;

using std::cout;
using std::endl;
using std::string;
using std::unique_ptr;
using std::shared_ptr;

namespace cpp2 {

IMCHandler::IMCHandler() {
  //LOG(ERROR) << "Start initializing";
  this->network_ = FLAGS_imc_network;
  this->weights_ = FLAGS_imc_weights;

  // load caffe model
  this->net_ = new Net<float>(this->network_);
  this->net_->CopyTrainedLayersFrom(this->weights_);
 
  LOG(ERROR) << "Finished initializing the handler!"; 
}

folly::Future<folly::Unit> IMCHandler::future_create
(unique_ptr<string> LUCID, unique_ptr< ::cpp2::QuerySpec> spec) {
	folly::MoveWrapper<folly::Promise<folly::Unit > > promise;
	auto future = promise->getFuture();
	promise->setValue(Unit{});
  LOG(ERROR) << "Create is not implemented now!"; 
	return future;
}

folly::Future<folly::Unit> IMCHandler::future_learn
(unique_ptr<string> LUCID, unique_ptr< ::cpp2::QuerySpec> knowledge) {
	//print("Learn");
	// Save LUCID and knowledge.
	folly::MoveWrapper<folly::Promise<folly::Unit>> promise;
	auto future = promise->getFuture();
	promise->setValue(Unit{});
  LOG(ERROR) << "Learn is not implemented now!"; 
	return future;
}

folly::Future<unique_ptr<string>> IMCHandler::future_infer
(unique_ptr<string> LUCID, unique_ptr< ::cpp2::QuerySpec> query) {
  //LOG(ERROR) << "start infer";
	string LUCID_save = *LUCID;
	::cpp2::QuerySpec query_save = *query;

  folly::MoveWrapper<folly::Promise<std::unique_ptr<std::string> > > promise;
  std::unique_ptr<std::string> image (new std::string(std::move(query_save.content[0].data[0])));
  
  auto move_image = folly::makeMoveWrapper(std::move(image));
  auto future = promise->getFuture();
  
  folly::RequestEventBase::get()->runInEventBaseThread(
      [promise, move_image, this]() mutable {
        // determine the image size
        int64_t img_size = 3 * 227 * 227;

        Caffe::set_phase(Caffe::TEST);
        // use cpu
        Caffe::set_mode(Caffe::CPU);

        int img_num = 1;

        // prepare data into array
        float* data = (float*) malloc(img_num * img_size * sizeof(float));
        float* preds = (float*) malloc(img_num * sizeof(float));

        //LOG(ERROR)<< "Read image!";

        unsigned char* buffer;
        // read in the image
        std::istringstream is(**move_image);
        is.seekg(0, is.end);
        int length = is.tellg();
        is.seekg(0, is.beg);
        char* img_buffer = new char[length];
        is.read(img_buffer, length);

        struct jpeg_decompress_struct cinfo;
        struct jpeg_error_mgr jerr;
        cinfo.err = jpeg_std_error(&jerr);
        jpeg_create_decompress(&cinfo);

        jpeg_mem_src(&cinfo, (unsigned char*)img_buffer, img_size);
        jpeg_read_header(&cinfo, TRUE);
        jpeg_start_decompress(&cinfo);

        img_size =
            cinfo.output_width * cinfo.output_height * cinfo.output_components;
        buffer = (unsigned char*) malloc(
            cinfo.output_components * cinfo.output_width);

        while (cinfo.output_scanline < cinfo.output_height) {
          (void) jpeg_read_scanlines(&cinfo, (JSAMPARRAY) &buffer, 1);
          for (int j = 0; j < cinfo.output_components; j++) {
            for (unsigned int k = 0; k < cinfo.output_width; k++) {
              int index = (cinfo.output_components -1 - j) *
                          cinfo.output_width * cinfo.output_height +
                          (cinfo.output_scanline - 1) * cinfo.output_width + k;
              data[index] = (float) buffer[k * cinfo.output_components + j];
            }
          }
        }
        
        delete[] img_buffer;
        delete[] buffer;
        jpeg_finish_decompress(&cinfo);
        jpeg_destroy_decompress(&cinfo);

        
        float loss;
        reshape(this->net_, img_num * img_size);

        std::vector<Blob<float>*> in_blobs = this->net_->input_blobs();
        in_blobs[0]->set_cpu_data(data);

        std::vector<Blob<float>*> out_blobs = this->net_->ForwardPrefilled(&loss);
        memcpy(preds, out_blobs[0]->cpu_data(), img_num * sizeof(float));

        std::unique_ptr<std::string> image_class = 
            folly::make_unique<std::string>(std::to_string(preds[0]));
        promise->setValue(std::move(image_class));
      }
  );

  return future;

}


/*
folly::Future<std::unique_ptr<std::string> >
IMCHandler::future_imageClassification(std::unique_ptr<std::string> image) {
  folly::MoveWrapper<folly::Promise<std::unique_ptr<std::string> > > promise;
  auto move_image = folly::makeMoveWrapper(std::move(image));
  auto future = promise->getFuture();
*/

void IMCHandler::reshape(Net<float>* net, int input_size) {
  int n_in = net->input_blobs()[0]->num();
  int c_in = net->input_blobs()[0]->channels();
  int w_in = net->input_blobs()[0]->width();
  int h_in = net->input_blobs()[0]->height();

  int n_out = net->output_blobs()[0]->num();
  int c_out = net->output_blobs()[0]->channels();
  int w_out = net->output_blobs()[0]->width();
  int h_out = net->output_blobs()[0]->height();

  // assumes C, H, W are known, only reshapes batch dim
  if (input_size / (c_in * w_in * h_in) != n_in) {
    n_in = input_size / (c_in * w_in * h_in);
    net->input_blobs()[0]->Reshape(n_in, c_in, w_in, h_in);

    n_out = n_in;
    net->output_blobs()[0]->Reshape(n_out, c_out, w_out, h_out);
  }
}


} // namespace cpp2
