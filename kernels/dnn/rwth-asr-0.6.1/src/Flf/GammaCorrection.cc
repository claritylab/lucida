// Copyright 2011 RWTH Aachen University. All rights reserved.
//
// Licensed under the RWTH ASR License (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.hltpr.rwth-aachen.de/rwth-asr/rwth-asr-license.html
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include <Core/Application.hh>
#include <Core/Parameter.hh>
#include <GammaCorrection.hh>


namespace Flf {

    // -------------------------------------------------------------------------
    namespace {
	f64 gammaCorrectionFunc(const f64 x, const f64 gamma, const f64 brpt = 0.3) {
	    if (x >= 1.0)
		return 1.0;
	    //const f64 y = 1.0 - ::pow(1.0 - x, gamma);
	    //return (y < 1e-12) ? 1e-12 : y;
	    if (x > brpt) {
		const f64 m1brpt = 1.0 - brpt;
		const f64 y = ((1.0 - ::pow(1.0 - ((x - brpt) / m1brpt), gamma)) * m1brpt) + brpt;
		return (y < 1e-12) ? 1e-12 : y;
	    } else {
		const f64 y = ::pow(x / brpt, gamma) * brpt;
		return (y < 1e-12) ? 1e-12 : y;
	    }
	}
    } // namespace
    // -------------------------------------------------------------------------

    // -------------------------------------------------------------------------
    void gammaCorrection(ConstConfusionNetworkRef cnRef, f64 gamma, bool normalize) {
	if (!cnRef || (gamma == 1.0))
	    return;
	if (!cnRef->isNormalized())
	    Core::Application::us()->criticalError("Confusion network pruning does only work for normalized CNs.");
	ConfusionNetwork &cn = const_cast<ConfusionNetwork&>(*cnRef);
	ScoreId posteriorId = cn.normalizedProperties->posteriorId;
	verify(posteriorId != Semiring::InvalidId);
	for (ConfusionNetwork::iterator itSlot = cn.begin(), endSlot = cn.end(); itSlot != endSlot; ++itSlot) {
	    ConfusionNetwork::Slot &slot = *itSlot;
	    Score sum = 0.0;
	    for (ConfusionNetwork::Slot::iterator itArc = slot.begin(), endArc = slot.end(); itArc != endArc; ++itArc) {
		Score &score = (*itArc->scores)[posteriorId];
		score = gammaCorrectionFunc(f64(score), gamma);
		sum += score;
	    }
	    if (normalize) {
		const Score norm = 1.0 / sum;
		for (ConfusionNetwork::Slot::iterator itArc = slot.begin(), endArc = slot.end(); itArc != endArc; ++itArc) {
		    Score &score = (*itArc->scores)[posteriorId];
		    score *= norm;
		}
	    }
	}
    }
    // -------------------------------------------------------------------------

    // -------------------------------------------------------------------------
    void gammaCorrection(ConstPosteriorCnRef cnRef, f64 gamma, bool normalize) {
	if (!cnRef || (gamma == 1.0))
	    return;
	PosteriorCn &cn = const_cast<PosteriorCn&>(*cnRef);
	for (PosteriorCn::iterator itSlot = cn.begin(), endSlot = cn.end(); itSlot != endSlot; ++itSlot) {
	    PosteriorCn::Slot &slot = *itSlot;
	    Probability sum = 0.0;
	    for (PosteriorCn::Slot::iterator itArc = slot.begin(), endArc = slot.end(); itArc != endArc; ++itArc) {
		Probability &score = itArc->score;
		score = gammaCorrectionFunc(f64(score), gamma);
		sum += score;
	    }
	    if (normalize) {
		const Probability norm = 1.0 / sum;
		for (PosteriorCn::Slot::iterator itArc = slot.begin(), endArc = slot.end(); itArc != endArc; ++itArc) {
		    Probability &score = itArc->score;
		    score *= norm;
		}
	    }
	}
    }
    // -------------------------------------------------------------------------

    // -------------------------------------------------------------------------
    class CnGammaCorrectionNode : public Node {
	typedef Node Precursor;
    public:
	static const Core::ParameterFloat paramGamma;
	static const Core::ParameterBool paramNormalize;
    protected:
	f64 gamma_;
	bool normalize_;
    public:
	CnGammaCorrectionNode(const std::string &name, const Core::Configuration &config) :
	    Node(name, config) {}
	virtual ~CnGammaCorrectionNode() {}
	virtual void init(const std::vector<std::string> &arguments) {
	    gamma_ = paramGamma(config);
	    // verify(gamma_ > 0.0);
	    if (gamma_ < 1e-6)
		gamma_ = 1e-6;
	    normalize_ = paramNormalize(config);
	    Core::Component::Message msg = log();
	    msg << "gamma=" << gamma_ << "\n";
	    if (normalize_)
		msg << "Re-normalize gamma corrected probability distribution\n";
	}
    };
    const Core::ParameterFloat CnGammaCorrectionNode::paramGamma(
	"gamma",
	"gamma",
	1.0);
    const Core::ParameterBool CnGammaCorrectionNode::paramNormalize(
	"normalize",
	"normalize",
	true);

    class NormalizedCnGammaCorrectionNode : public CnGammaCorrectionNode {
	typedef CnGammaCorrectionNode Precursor;
    public:
	NormalizedCnGammaCorrectionNode(const std::string &name, const Core::Configuration &config) :
	    Precursor(name, config) {}
	virtual ~NormalizedCnGammaCorrectionNode() {}

	virtual ConstConfusionNetworkRef sendCn(Port to) {
	    verify(connected(to));
	    ConstConfusionNetworkRef cn = requestCn(to);
	    if (cn)
		gammaCorrection(cn, gamma_);
	    return cn;
	}
    };
    NodeRef createNormalizedCnGammaCorrectionNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new NormalizedCnGammaCorrectionNode(name, config));
    }

    class PosteriorCnGammaCorrectionNode : public CnGammaCorrectionNode {
	typedef CnGammaCorrectionNode Precursor;
    public:
	PosteriorCnGammaCorrectionNode(const std::string &name, const Core::Configuration &config) :
	    Precursor(name, config) {}
	virtual ~PosteriorCnGammaCorrectionNode() {}

	virtual ConstPosteriorCnRef sendPosteriorCn(Port to) {
	    verify(connected(to));
	    ConstPosteriorCnRef cn = requestPosteriorCn(to);
	    if (cn)
		gammaCorrection(cn, gamma_);
	    return cn;
	}
    };
    NodeRef createPosteriorCnGammaCorrectionNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new PosteriorCnGammaCorrectionNode(name, config));
    }
    // -------------------------------------------------------------------------

} // namespace Flf
