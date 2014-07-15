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
#include "Core/Application.hh"

#include "FlfCore/Traverse.hh"
#include "TimeframeConfusionNetwork.hh"
#include "TimeframeConfusionNetworkBuilder.hh"

namespace Flf {

    // -------------------------------------------------------------------------
    namespace {
		void scoresToProbabilities(PosteriorCn &cn, bool normalize = false) {
			for (u32 i = 0; i < cn.size(); ++i) {
				PosteriorCn::Slot &pdf(cn[i]);
				Probability sum = 0.0;
				for (PosteriorCn::Slot::iterator itPdf = pdf.begin(); itPdf != pdf.end(); ++itPdf)
					sum += (itPdf->score = ::exp(-itPdf->score));
				verify(sum > 0.0);
				if (!normalize)
					if ((sum <= 0.99) || (1.01 <= sum))
						Core::Application::us()->warning(
							"Numerical instability: sum over posterior CN slot exceeds [0.99, 1.01]; "
							"sum is %f.", sum);
				// Normalize anyway to deal with slight numerical instabilities, e.g. rounding errors
				for (PosteriorCn::Slot::iterator itPdf = pdf.begin(); itPdf != pdf.end(); ++itPdf)
					itPdf->score /= sum;
			}
		}
    } // namespace
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    /**
     * Calculate time frame wise word posterior distribution from
     * arc confidence score, i.e. p(w|t) =
     * (sum_{e \in E: begin(e) <= t < end(e) and word(e) = w} conf(e))
     * / (sum_{e \in E: begin(e) <= t < end(e)} conf(e))
     *
     * The result is a list of posterior distributions p(*|t) for t = 1,...,T
     **/
    template<class ArcScores>
    class FramewiseCollector : public TraverseState {
    private:
		ArcScores arcScores_;
		PosteriorCn &cn_;
		Time begin_, end_;
    protected:
		virtual void exploreState(ConstStateRef sr) {
			typename ArcScores::const_iterator itScore = arcScores_(sr->id()).first;
			for (State::const_iterator a = sr->begin(), end_a = sr->end();
				 a != end_a; ++a, ++itScore) {
				if (a->input() == Fsa::InvalidLabelId)
					Core::Application::us()->criticalError("Frame-wise score collector: label id is invalid.");
				PosteriorCn::Arc cnArc(a->input(), itScore->score());
				Time t = l->boundary(sr->id()).time(), end_t = l->boundary(a->target()).time();
				if ((t == InvalidTime) || (end_t == InvalidTime) || (t > end_t) || (end_t >= 2147483647))
					Core::Application::us()->criticalError("Frame-wise score collector: interval [%d,%d] is invalid.", t, end_t);

				if (t < end_t) {
					begin_ = std::min(begin_, t);
					end_ = std::max(end_, end_t);
					cn_.grow(end_t - 1);
					for (; t < end_t; ++t) {
						PosteriorCn::Slot &pdf = cn_[t];
						PosteriorCn::Slot::iterator pdfIt = std::lower_bound(
							pdf.begin(), pdf.end(), cnArc);
						if ((pdfIt == pdf.end()) || (pdfIt->label != cnArc.label)) {

							// dbg(t << ": insert " << l->getInputAlphabet()->symbol(cnArc.label) << "/" << cnArc.label);

							pdf.insert(pdfIt, cnArc);
						} else
							pdfIt->score = ArcScores::collect(pdfIt->score, cnArc.score);
					}
				}
			}
		}
    public:
		FramewiseCollector(
			PosteriorCn &cn,	//
			ConstLatticeRef l,
			const ArcScores &arcScores) :
			TraverseState(l), arcScores_(arcScores), cn_(cn), begin_(Core::Type<Time>::max), end_(0) {
			if (!l->getBoundaries()->valid())
				Core::Application::us()->criticalError(
					"FramewiseCollector: Lattice \"%s\" has no time boundaries",
					l->describe().c_str());
			traverse();
			PosteriorCn::Slot epsSlot(1, PosteriorCn::Arc(Fsa::Epsilon, 0.0));
			for (u32 i = 0; (i < cn_.size()) && cn_[i].empty(); ++i)
				cn_[i] = epsSlot;
		}
		virtual ~FramewiseCollector() {}
		Time begin() const { return begin_; }
		Time end()   const { return end_; }
    };
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class FwdBwdScorer {
    public:
		typedef FwdBwd::State::const_iterator const_iterator;
    private:
		ConstFwdBwdRef fb_;
    public:
		FwdBwdScorer(ConstFwdBwdRef fb) : fb_(fb) {}
		std::pair<const_iterator, const_iterator> operator() (Fsa::StateId sid) const {
			const FwdBwd::State &state  = fb_->state(sid);
			return std::make_pair(state.begin(), state.end());
		}
		static f64 collect(f64 score1, f64 score2) {
			return std::min(score1, score2) -
				::log1p(::exp(std::min(score1, score2) - std::max(score1, score2)));
		}
    };

    ConstPosteriorCnRef buildFramePosteriorCn(ConstLatticeRef l, ConstFwdBwdRef fb) {
		verify(fb);
		PosteriorCn *cn = new PosteriorCn;
		cn->alphabet = l->getInputAlphabet();
		FramewiseCollector<FwdBwdScorer> fCol(*cn, l, FwdBwdScorer(fb));
		scoresToProbabilities(*cn, false);
		return ConstPosteriorCnRef(cn);
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class PrecalculatedArcScores {
    public:
		struct const_iterator {
			State::const_iterator itArc;
			ScoreId id;
			const_iterator(State::const_iterator itArc, ScoreId id) :
				itArc(itArc), id(id) {}
			void operator++ ()
				{ ++itArc; }
			bool operator!= (const const_iterator &it) const
				{ return itArc != it.itArc; }
			const const_iterator * operator->() const
				{ return this; }
			Probability score() const
				{ return itArc->score(id); }
		};
    private:
		ConstLatticeRef l_;
		ScoreId id_;
    public:
		PrecalculatedArcScores(ConstLatticeRef l, ScoreId id) :
			l_(l), id_(id) {}
		std::pair<const_iterator, const_iterator>
		operator() (Fsa::StateId sid) const {
			ConstStateRef sr = l_->getState(sid);
			return std::make_pair(
				const_iterator(sr->begin(), id_),
				const_iterator(sr->end(), Semiring::InvalidId));
		}
		static f64 collect(f64 score1, f64 score2) {
			return std::min(score1, score2) -
				::log1p(::exp(std::min(score1, score2) - std::max(score1, score2)));
		}
    };

    ConstPosteriorCnRef buildFramePosteriorCn(ConstLatticeRef l, ScoreId id) {
		verify(id != Semiring::InvalidId);
		PosteriorCn *cn = new PosteriorCn();
		PrecalculatedArcScores arcScorer(l, id);
		FramewiseCollector<PrecalculatedArcScores> fCol(*cn, l, arcScorer);
		scoresToProbabilities(*cn, true);
		return ConstPosteriorCnRef(cn);
    }
    // -------------------------------------------------------------------------



    // -------------------------------------------------------------------------
    /**
     * No support for pre-calculated scoers yet. Needed??
     **/
    class FramePosteriorCnBuilderNode : public Node {
    private:
		u32 n_;
		FwdBwdBuilderRef fbBuilder_;
		ConstLatticeRef l_;
		ConstPosteriorCnRef cn_;
		bool hasNext_;

    private:
		void next() {
			if (!hasNext_) {
				ConstLatticeRefList lats(n_);
				for (u32 i = 0; i < n_; ++i) {
					ConstLatticeRef l = requestLattice(i);
					if (!l) {
						warning("No lattice provided at port %d; discard", i);
						lats.clear();
						break;
					}
					lats[i] = l;
				}
				if (!lats.empty()) {
					std::pair<ConstLatticeRef, ConstFwdBwdRef> fbResult = (n_ == 1) ?
						fbBuilder_->build(lats.front()) : fbBuilder_->build(lats);
					l_ = fbResult.first;
					cn_ = buildFramePosteriorCn(fbResult.first, fbResult.second);
				}
				hasNext_ = true;
			}
		}

    public:
		FramePosteriorCnBuilderNode(const std::string &name, const Core::Configuration &config) :
			Node(name, config), n_(0) {}
		virtual ~FramePosteriorCnBuilderNode() {}

		virtual void init(const std::vector<std::string> &arguments) {
			for (n_ = 0; connected(n_); ++n_);
			if (n_ == 0)
				criticalError("At least one incoming lattice at port 0 required.");
			if (n_ > 1)
				log() << "Combine " << n_ << " lattices.\n\n";
			fbBuilder_ = FwdBwdBuilder::create(select("fb"));
			hasNext_ = false;
		}

		virtual ConstLatticeRef sendLattice(Port to) {
			next();
			switch (to) {
			case 0:
				return l_;
			case 2:
				return posteriorCn2lattice(cn_);
			default:
				defect();
				return ConstLatticeRef();
			}
		}

		virtual ConstPosteriorCnRef sendPosteriorCn(Port to) {
			verify(to == 1);
			next();
			return cn_;
		}

		virtual void sync() {
			l_.reset();
			cn_.reset();
			hasNext_ = false;
		}
    };
    NodeRef createFramePosteriorCnBuilderNode(const std::string &name, const Core::Configuration &config) {
		return NodeRef(new FramePosteriorCnBuilderNode(name, config));
    }
    // -------------------------------------------------------------------------

} // namespace Flf
