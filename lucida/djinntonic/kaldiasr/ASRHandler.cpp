#include "ASRHandler.h"

#include <folly/futures/Future.h>

#include <sstream>
#include <unistd.h>

//#include <utility>
//#include <thrift/lib/cpp2/async/HeaderClientChannel.h>
//#include <thrift/lib/cpp2/server/ThriftServer.h>

#include "src/online2/online-nnet2-decoding.h"
#include "src/online2/onlinebin-util.h"
#include "src/online2/online-timing.h"
#include "src/online2/online-endpoint.h"
#include "src/fstext/fstext-lib.h"
#include "src/lat/lattice-functions.h"
#include "src/thread/kaldi-thread.h"

DEFINE_string(word_symbol_table,
              "model/words.txt",
              "Config for word symbol table (default: model/words.txt");

DEFINE_string(nnet_filename,
              "model/smbr_epoch2.mdl",
              "Config file for nnet (default: model/smbr_epoch2.mdl)");

DEFINE_string(fst_filename,
              "model/HCLG.fst",
              "Config file for FST (default: model/HCLG.fst)");

// feature_config
DEFINE_string(feature_type,
              "mfcc",
              "Config for base feature (default: mfcc)");

DEFINE_string(mfcc_config,
              "model/mfcc.conf",
              "Config file for mfcc feature (default: model/mfcc.conf)");

DEFINE_string(ivector_extraction_config,
              "model/ivector_extractor.conf",
              "Config file for ivector extraction "
              "(default: model/ivector_extractor.conf)");

// nnet_decoding_config
DEFINE_double(beam,
              15.0,
              "Config for beam (default: 15.0)");

DEFINE_int32(max_active,
             7000,
             "Config for max active (default: 7000)");

DEFINE_double(lattice_beam,
              6.0,
              "Config for lattice beam (default: 6.0)");

DEFINE_double(acoustic_scale,
              0.1,
              "Config for acoustic scale (default: 0.1)");

// endpoint_config
DEFINE_string(silence_phones,
              "1:2:3:4:5:6:7:8:9:10:11:12:13:14:15:16:17:18:19:20",
              "Config for silence phones (default: "
              "1:2:3:4:5:6:7:8:9:10:11:12:13:14:15:16:17:18:19:20)");
//using namespace std;
//using namespace apache::thrift;
//using namespace apache::thrift::async;

using namespace folly;
using std::cout;
using std::endl;
using std::string;
using std::unique_ptr;
using std::shared_ptr;

/*
namespace facebook {
namespace windtunnel {
namespace treadmill {
namespace services {
namespace asr {
*/

namespace cpp2{

// integer types defined in kaldi
typedef kaldi::int32 int32;
typedef kaldi::int64 int64;

std::string ASRHandler::getDiagnostics(const fst::SymbolTable* word_syms,
                                       const kaldi::CompactLattice& clat) {
  kaldi::CompactLattice best_path_clat;
  kaldi::CompactLatticeShortestPath(clat, &best_path_clat);

  kaldi::Lattice best_path_lat;
  fst::ConvertLattice(best_path_clat, &best_path_lat);

  kaldi::LatticeWeight weight;
  std::vector<int32> alignment;
  std::vector<int32> words;
  GetLinearSymbolSequence(best_path_lat, &alignment, &words, &weight);

  std::string text;
  for (unsigned int i = 0; i < words.size(); i++) {
    text += word_syms->Find(words[i]) + " ";
  }
  return text;
}

/**
 * all the initialization
 * TODO: replicate this per thread
 */
ASRHandler::ASRHandler() {
  // feature_config
  // nnet2_decoding_config
  // endpoint_config
  kaldi::OnlineEndpointConfig endpoint_config;

  // command line configs
  this->chunk_length_secs_ = 0.05;
  std::string word_symbol_table = FLAGS_word_symbol_table;
  this->do_endpointing_ = false;
  this->online_ = false;

  // set configs for feature_config
  this->feature_config_.feature_type = FLAGS_feature_type;
  this->feature_config_.mfcc_config = FLAGS_mfcc_config;
  this->feature_config_.ivector_extraction_config =
      FLAGS_ivector_extraction_config;

  // set configs for nnet_decoding_config
  // decoder_opts
  this->nnet2_decoding_config_.decoder_opts.beam = FLAGS_beam;
  this->nnet2_decoding_config_.decoder_opts.max_active = FLAGS_max_active;
  this->nnet2_decoding_config_.decoder_opts.lattice_beam = FLAGS_lattice_beam;
  // decodable_opts
  this->nnet2_decoding_config_.decodable_opts.acoustic_scale =
      FLAGS_acoustic_scale;

  // set configs for endpoint_config
  endpoint_config.silence_phones = FLAGS_silence_phones;

  std::string nnet2_rxfilename = FLAGS_nnet_filename;
  std::string fst_rxfilename = FLAGS_fst_filename;

  bool binary;
  kaldi::Input ki(nnet2_rxfilename, &binary);
  this->trans_model_.Read(ki.Stream(), binary);
  this->nnet_.Read(ki.Stream(), binary);
    
  this->decode_fst_ = fst::ReadFstKaldi(fst_rxfilename);

  if (word_symbol_table != "") {
    if (!(this->word_syms_ = fst::SymbolTable::ReadText(word_symbol_table))) {
      LOG(ERROR) << "Could not read symbol table from file "
                 << word_symbol_table;
    }
  }

  LOG(INFO) << "Done initializing the handler!";
}

folly::Future<folly::Unit> ASRHandler::future_create
(unique_ptr<string> LUCID, unique_ptr< ::cpp2::QuerySpec> spec) {
	folly::MoveWrapper<folly::Promise<folly::Unit > > promise;
	auto future = promise->getFuture();
	promise->setValue(Unit{});
	return future;
}

folly::Future<folly::Unit> ASRHandler::future_learn
(unique_ptr<string> LUCID, unique_ptr< ::cpp2::QuerySpec> knowledge) {
	//print("Learn");
	// Save LUCID and knowledge.
	folly::MoveWrapper<folly::Promise<folly::Unit>> promise;
	auto future = promise->getFuture();
	promise->setValue(Unit{});
	return future;
}

folly::Future<unique_ptr<string>> ASRHandler::future_infer
(unique_ptr<string> LUCID, unique_ptr< ::cpp2::QuerySpec> query) {
	//print("Infer");
	// Save LUCID and query.
	string LUCID_save = *LUCID;
	::cpp2::QuerySpec query_save = *query;

  folly::MoveWrapper<folly::Promise<std::unique_ptr<std::string> > > promise;

  std::unique_ptr<std::string> audio (new std::string(std::move(query_save.content[0].data[0])));
  
  //std::string audio_save = query_save.content[0].data[0];
  //std::unique_ptr<std::string> audio (audio_save); // first data for the first content
  //std::unique_ptr<std::string> audio (&query_save.content[0].data[0]); // first data for the first content
  
  auto move_audio = folly::makeMoveWrapper(std::move(audio));
  auto future = promise->getFuture();
  
  folly::RequestEventBase::get()->runInEventBaseThread(
      [promise, move_audio, this]() mutable {
        kaldi::WaveData wave_data;
        std::istringstream is(**move_audio);
        wave_data.Read(is);

        kaldi::SubVector<kaldi::BaseFloat> data(wave_data.Data(), 0);

        kaldi::OnlineNnet2FeaturePipelineInfo feature_info(this->feature_config_);
        if (!this->online_) {
          feature_info.ivector_extractor_info.use_most_recent_ivector = true;
          feature_info.ivector_extractor_info.greedy_ivector_extractor = true;
          this->chunk_length_secs_ = -1.0;
        }

        kaldi::OnlineNnet2FeaturePipeline feature_pipeline(feature_info);
        kaldi::OnlineIvectorExtractorAdaptationState adaptation_state(
            feature_info.ivector_extractor_info);
        feature_pipeline.SetAdaptationState(adaptation_state);

        kaldi::OnlineSilenceWeighting silence_weighting(
            this->trans_model_,
            feature_info.silence_weighting_config);

        kaldi::SingleUtteranceNnet2Decoder decoder(this->nnet2_decoding_config_,
                                            this->trans_model_,
                                            this->nnet_,
                                            *this->decode_fst_,
                                            &feature_pipeline);

        kaldi::BaseFloat samp_freq = wave_data.SampFreq();
        int32 chunk_length;
        if (this->chunk_length_secs_ > 0) {
          chunk_length = int32(samp_freq * this->chunk_length_secs_);
          if (chunk_length == 0) {
            chunk_length = 1;
          }
        } else {
          chunk_length = std::numeric_limits<int32>::max();
        }
        
        int32 samp_offset = 0;
        std::vector<std::pair<int32, kaldi::BaseFloat> > delta_weights;

        int32 samp_remaining = data.Dim();
        int32 num_samp = chunk_length < samp_remaining ? chunk_length :
                                                         samp_remaining;
        kaldi::SubVector<kaldi::BaseFloat> wave_part(data, samp_offset, num_samp);
        feature_pipeline.AcceptWaveform(samp_freq, wave_part);

        samp_offset += num_samp;
        feature_pipeline.InputFinished();

        if (silence_weighting.Active()) {
          silence_weighting.ComputeCurrentTraceback(decoder.Decoder());
          silence_weighting.GetDeltaWeights(feature_pipeline.NumFramesReady(),
                                            &delta_weights);
          feature_pipeline.UpdateFrameWeights(delta_weights);
        }

        decoder.AdvanceDecoding();
        decoder.FinalizeDecoding();

        kaldi::CompactLattice clat;
        decoder.GetLattice(true, &clat);

        std::unique_ptr<std::string> result =
            folly::make_unique<std::string>(
                this->getDiagnostics(this->word_syms_, clat));
        promise->setValue(std::move(result));
      }
  );



  return future;
}

/*
folly::Future<std::unique_ptr<std::string> >
ASRHandler::future_speechRecognition(std::unique_ptr<std::string> audio) {
  folly::MoveWrapper<folly::Promise<std::unique_ptr<std::string> > > promise;
  auto move_audio = folly::makeMoveWrapper(std::move(audio));
  auto future = promise->getFuture();

  folly::RequestEventBase::get()->runInEventBaseThread(
      [promise, move_audio, this]() mutable {
        kaldi::WaveData wave_data;
        std::istringstream is(**move_audio);
        wave_data.Read(is);

        kaldi::SubVector<kaldi::BaseFloat> data(wave_data.Data(), 0);

        kaldi::OnlineNnet2FeaturePipelineInfo feature_info(this->feature_config_);
        if (!this->online_) {
          feature_info.ivector_extractor_info.use_most_recent_ivector = true;
          feature_info.ivector_extractor_info.greedy_ivector_extractor = true;
          this->chunk_length_secs_ = -1.0;
        }

        kaldi::OnlineNnet2FeaturePipeline feature_pipeline(feature_info);
        kaldi::OnlineIvectorExtractorAdaptationState adaptation_state(
            feature_info.ivector_extractor_info);
        feature_pipeline.SetAdaptationState(adaptation_state);

        kaldi::OnlineSilenceWeighting silence_weighting(
            this->trans_model_,
            feature_info.silence_weighting_config);

        kaldi::SingleUtteranceNnet2Decoder decoder(this->nnet2_decoding_config_,
                                            this->trans_model_,
                                            this->nnet_,
                                            *this->decode_fst_,
                                            &feature_pipeline);

        kaldi::BaseFloat samp_freq = wave_data.SampFreq();
        int32 chunk_length;
        if (this->chunk_length_secs_ > 0) {
          chunk_length = int32(samp_freq * this->chunk_length_secs_);
          if (chunk_length == 0) {
            chunk_length = 1;
          }
        } else {
          chunk_length = std::numeric_limits<int32>::max();
        }
        
        int32 samp_offset = 0;
        std::vector<std::pair<int32, kaldi::BaseFloat> > delta_weights;

        int32 samp_remaining = data.Dim();
        int32 num_samp = chunk_length < samp_remaining ? chunk_length :
                                                         samp_remaining;
        kaldi::SubVector<kaldi::BaseFloat> wave_part(data, samp_offset, num_samp);
        feature_pipeline.AcceptWaveform(samp_freq, wave_part);

        samp_offset += num_samp;
        feature_pipeline.InputFinished();

        if (silence_weighting.Active()) {
          silence_weighting.ComputeCurrentTraceback(decoder.Decoder());
          silence_weighting.GetDeltaWeights(feature_pipeline.NumFramesReady(),
                                            &delta_weights);
          feature_pipeline.UpdateFrameWeights(delta_weights);
        }

        decoder.AdvanceDecoding();
        decoder.FinalizeDecoding();

        kaldi::CompactLattice clat;
        decoder.GetLattice(true, &clat);

        std::unique_ptr<std::string> result =
            folly::make_unique<std::string>(
                this->getDiagnostics(this->word_syms_, clat));
        promise->setValue(std::move(result));
      }
  );

  return future;
}
*/


} // namespace cpp2

/*
} // namespace asr
} // namespace services
} // namespace treadmill
} // namespace windtunnel
} // namespace facebook
*/
