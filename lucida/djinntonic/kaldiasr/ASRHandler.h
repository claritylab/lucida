#pragma once

//#include "gen-cpp2/ASR.h"
#include "gen-cpp2/LucidaService.h"

#include "src/nnet2/am-nnet.h"
#include "src/feat/wave-reader.h"
#include "src/online2/online-nnet2-feature-pipeline.h"
#include "src/online2/online-nnet2-decoding.h"

/*
namespace facebook {
namespace windtunnel {
namespace treadmill {
namespace services {
namespace asr {
*/

namespace cpp2 {

//class ASRHandler : virtual public ASRSvIf {
class ASRHandler : virtual public LucidaServiceSvIf {
 public:
  ASRHandler();


  folly::Future<folly::Unit> future_create
  (std::unique_ptr<std::string> LUCID,
      std::unique_ptr< ::cpp2::QuerySpec> spec);

  folly::Future<folly::Unit> future_learn
  (std::unique_ptr<std::string> LUCID,
      std::unique_ptr< ::cpp2::QuerySpec> knowledge);

  folly::Future<std::unique_ptr<std::string>> future_infer
  (std::unique_ptr<std::string> LUCID,
      std::unique_ptr< ::cpp2::QuerySpec> query);


/*
  folly::Future<std::unique_ptr<std::string> >
  future_speechRecognition(std::unique_ptr<std::string> audio);
*/
 private:
  std::string getDiagnostics(const fst::SymbolTable* word_syms,
                             const kaldi::CompactLattice& clat);

  kaldi::OnlineNnet2DecodingConfig nnet2_decoding_config_;
  kaldi::OnlineNnet2FeaturePipelineConfig feature_config_;
  kaldi::TransitionModel trans_model_;
  kaldi::nnet2::AmNnet nnet_;
  fst::Fst<fst::StdArc>* decode_fst_;
  fst::SymbolTable* word_syms_;
  kaldi::BaseFloat chunk_length_secs_;
  bool online_;
  bool do_endpointing_;
};

} // namespace cpp2

/*
} // namespace asr
} // namespace services
} // namespace treadmill
} // namespace windtunnel
} // namespace facebook
*/
