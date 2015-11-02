#ifndef VIGRA_RF_RANDOM_FOREST_COMMON_HXX
#define VIGRA_RF_RANDOM_FOREST_COMMON_HXX

#include <iterator>
#include <type_traits>
#include <cmath>
#include <algorithm>

#include "../multi_array.hxx"
#include "../mathutil.hxx"

namespace vigra
{



template <typename T>
struct LessEqualSplitTest
{
public:
    LessEqualSplitTest(size_t dim = 0, T const & val = 0)
        :
        dim_(dim),
        val_(val)
    {}
    size_t operator()(MultiArrayView<1, T> const & features) const
    {
        return features(dim_) <= val_ ? 0 : 1;
    }
    size_t dim_;
    T val_;
};



struct ArgMaxAcc
{
public:
    typedef size_t input_type;

    template <typename ITER, typename OUTITER>
    void operator()(ITER begin, ITER end, OUTITER out)
    {
        std::fill(buffer_.begin(), buffer_.end(), 0);
        size_t max_v = 0;
        size_t n = 0;
        for (ITER it = begin; it != end; ++it)
        {
            size_t const v = *it;
            if (v >= buffer_.size())
            {
                buffer_.resize(v+1, 0);
            }
            ++buffer_[v];
            ++n;
            max_v = std::max(max_v, v);
        }
        for (size_t i = 0; i <= max_v; ++i)
        {
            *out = buffer_[i] / static_cast<double>(n);
            ++out;
        }
    }
private:
    std::vector<size_t> buffer_;
};



template <typename VALUETYPE>
struct ArgMaxVectorAcc
{
public:
    typedef VALUETYPE value_type;
    typedef std::vector<value_type> input_type;
    template <typename ITER, typename OUTITER>
    void operator()(ITER begin, ITER end, OUTITER out)
    {
        std::fill(buffer_.begin(), buffer_.end(), 0);
        size_t max_v = 0;
        for (ITER it = begin; it != end; ++it)
        {
            input_type const & vec = *it;
            if (vec.size() >= buffer_.size())
            {
                buffer_.resize(vec.size(), 0);
            }
            value_type const n = std::accumulate(vec.begin(), vec.end(), static_cast<value_type>(0));
            for (size_t i = 0; i < vec.size(); ++i)
            {
                buffer_[i] += vec[i] / static_cast<double>(n);
            }
            max_v = std::max(vec.size()-1, max_v);
        }
        for (size_t i = 0; i <= max_v; ++i)
        {
            *out = buffer_[i];
            ++out;
        }
    }
    private:
        std::vector<double> buffer_;
};



// struct LargestSumAcc
// {
// public:
//     typedef std::vector<size_t> input_type; 
//     template <typename ITER>
//     size_t operator()(ITER begin, ITER end)
//     {
//         std::fill(buffer_.begin(), buffer_.end(), 0);
//         for (ITER it = begin; it != end; ++it)
//         {
//             auto const & v = *it;
//             if (v.size() > buffer_.size())
//             {
//                 buffer_.resize(v.size(), 0);
//             }
//             for (size_t i = 0; i < v.size(); ++i)
//             {
//                 buffer_[i] += v[i];
//             }
//         }
//         size_t max_label = 0;
//         size_t max_count = 0;
//         for (size_t i = 0; i < buffer_.size(); ++i)
//         {
//             if (buffer_[i] > max_count)
//             {
//                 max_count = buffer_[i];
//                 max_label = i;
//             }
//         }
//         return max_label;
//     }
// private:
//     std::vector<size_t> buffer_;
// };



// struct ForestGarroteAcc
// {
// public:
//     typedef double input_type;
//     template <typename ITER, typename OUTITER>
//     void operator()(ITER begin, ITER end, OUTITER out)
//     {
//         double s = 0.0;
//         for (ITER it = begin; it != end; ++it)
//         {
//             s += *it;
//         }
//         if (s < 0.0)
//             s = 0.0;
//         else if (s > 1.0)
//             s = 1.0;
//         *out = 1.0-s;
//         ++out;
//         *out = s;
//     }
// };



namespace detail
{

    template <bool MINIMIZE, typename FUNCTOR>
    class GeneralScorer
    {
    public:

        GeneralScorer(std::vector<size_t> const & priors)
            :
            split_found_(false),
            best_split_(0),
            best_dim_(0),
            best_score_(MINIMIZE ? std::numeric_limits<double>::max() : std::numeric_limits<double>::lowest()),
            priors_(priors),
            n_total_(std::accumulate(priors.begin(), priors.end(), 0))
        {}

        template <typename FEATURES, typename LABELS, typename WEIGHTS, typename ITER>
        void operator()(
            FEATURES const & features,
            LABELS const & labels,
            WEIGHTS const & weights,
            ITER begin,
            ITER end,
            size_t dim
        ){
            if (begin == end)
                return;

            FUNCTOR score;

            std::vector<size_t> counts(priors_.size(), 0);
            size_t n_left = 0;
            ITER next = begin;
            ++next;
            for (; next != end; ++begin, ++next)
            {
                // Move the label from the right side to the left side.
                size_t const left_index = *begin;
                size_t const right_index = *next;
                size_t const label = static_cast<size_t>(labels(left_index));
                counts[label] += weights[left_index];
                n_left += weights[left_index];

                // Skip if there is no new split.
                auto const left = features(left_index, dim);
                auto const right = features(right_index, dim);
                if (left == right)
                    continue;

                // Update the score.
                split_found_ = true;
                double const s = score(priors_, counts, n_total_, n_left);
                bool const better_score = MINIMIZE ? s < best_score_ : s > best_score_;
                if (better_score)
                {
                    best_score_ = s;
                    best_split_ = 0.5*(left+right);
                    best_dim_ = dim;
                }
            }
        }

        bool split_found_;
        double best_split_;
        size_t best_dim_;
        double best_score_;

    private:

        std::vector<size_t> const priors_;
        size_t n_total_;
    };

    class GiniScoreFunctor
    {
    public:
        double operator()(std::vector<size_t> const & priors, std::vector<size_t> const & counts, double n_total, double n_left) const
        {
            double const n_right = n_total - n_left;
            double gini_left = 1;
            double gini_right = 1;
            for (size_t i = 0; i < counts.size(); ++i)
            {
                double const p_left = counts[i] / n_left;
                double const p_right = (priors[i] - counts[i]) / n_right;
                gini_left -= (p_left*p_left);
                gini_right -= (p_right*p_right);
            }
            return n_left*gini_left + n_right*gini_right;
        }
    };

    class EntropyScoreFunctor
    {
    public:
        double operator()(std::vector<size_t> const & priors, std::vector<size_t> const & counts, double n_total, double n_left) const
        {
            double const n_right = n_total - n_left;
            double ig = 0;
            for (size_t i = 0; i < counts.size(); ++i)
            {
                auto c = counts[i];
                if (c != 0)
                    ig -= c * std::log(c / n_left);

                c = priors[i] - c;
                if (c != 0)
                    ig -= c * std::log(c / n_right);
            }
            return ig;
        }
    };

    class KSDScoreFunctor
    {
    public:
        double operator()(std::vector<size_t> const & priors, std::vector<size_t> const & counts, double n_total, double n_left) const
        {
            size_t nnz = 0;
            std::vector<double> norm_counts(counts.size(), 0);
            for (size_t i = 0; i < counts.size(); ++i)
            {
                if (priors[i] != 0)
                {
                    norm_counts[i] = counts[i] / static_cast<double>(priors[i]);
                    ++nnz;
                }
            }
            if (nnz == 0)
                return 0.0;

            // NOTE to future self:
            // In std::accumulate, it makes a huge difference whether you use 0 or 0.0 as init. Think about that before making changes.
            double const mean = std::accumulate(norm_counts.begin(), norm_counts.end(), 0.0) / static_cast<double>(nnz);

            // Compute the sum of the squared distances.
            double ksd = 0.0;
            for (size_t i = 0; i < norm_counts.size(); ++i)
            {
                if (priors[i] != 0)
                {
                    double const v = (mean-norm_counts[i]);
                    ksd += v*v;
                }
            }
            return ksd;
        }
    };

} // namespace detail

typedef detail::GeneralScorer<true, detail::GiniScoreFunctor> GiniScorer;
typedef detail::GeneralScorer<true, detail::EntropyScoreFunctor> EntropyScorer;
typedef detail::GeneralScorer<false, detail::KSDScoreFunctor> KSDScorer;



template <typename ARR>
struct RFNodeDescription
{
public:
    RFNodeDescription(size_t depth, ARR const & priors)
        :
        depth_(depth),
        priors_(priors)
    {}
    size_t depth_;
    ARR const & priors_;
};



template <typename LABELS, typename ITER>
bool is_pure(LABELS const & labels, RFNodeDescription<ITER> const & desc)
{
    bool found = false;
    for (auto n : desc.priors_)
    {
        if (n > 0)
        {
            if (found)
                return false;
            else
                found = true;
        }
    }
    return true;
}



class PurityStop
{
public:
    template <typename LABELS, typename ITER>
    bool operator()(LABELS const & labels, RFNodeDescription<ITER> const & desc) const
    {
        return is_pure(labels, desc);
    }
};



class DepthStop
{
public:
    DepthStop(size_t max_depth)
        :
        max_depth_(max_depth)
    {}
    template <typename LABELS, typename ITER>
    bool operator()(LABELS const & labels, RFNodeDescription<ITER> const & desc) const
    {
        if (desc.depth_ >= max_depth_)
            return true;
        else
            return is_pure(labels, desc);
    }
    size_t max_depth_;
};



class NumInstancesStop
{
public:
    NumInstancesStop(size_t min_n)
        :
        min_n_(min_n)
    {}
    template <typename LABELS, typename ARR>
    bool operator()(LABELS const & labels, RFNodeDescription<ARR> const & desc) const
    {
        typedef typename ARR::value_type value_type;
        if (std::accumulate(desc.priors_.begin(), desc.priors_.end(), static_cast<value_type>(0)) <= min_n_)
            return true;
        else
            return is_pure(labels, desc);
    }
    size_t min_n_;
};



class NodeComplexityStop
{
public:
    NodeComplexityStop(double tau = 0.001)
        :
        logtau_(std::log(tau))
    {
        vigra_precondition(tau > 0 && tau < 1, "NodeComplexityStop(): Tau must be in the open interval (0, 1).");
    }

    template <typename LABELS, typename ARR>
    bool operator()(LABELS const & labels, RFNodeDescription<ARR> const & desc)
    {
        typedef typename ARR::value_type value_type;

        // Count the labels.
        size_t const total = std::accumulate(desc.priors_.begin(), desc.priors_.end(), static_cast<value_type>(0));

        // Compute log(prod_k(n_k!)).
        size_t nnz = 0;
        double lg = 0.0;
        for (auto v : desc.priors_)
        {
            if (v > 0)
            {
                ++nnz;
                lg += loggamma(static_cast<double>(v+1));
            }
        }
        lg += loggamma(static_cast<double>(nnz+1));
        lg -= loggamma(static_cast<double>(total+1));
        if (nnz <= 1)
            return true;

        return lg >= logtau_;
    }

    double logtau_;
};



enum RandomForestOptionTags
{
    RF_SQRT,
    RF_LOG,
    RF_CONST,
    RF_ALL,
    RF_GINI,
    RF_ENTROPY,
    RF_KSD
};



class RandomForestNewOptions
{
public:

    RandomForestNewOptions()
        :
        tree_count_(256),
        features_per_node_(0),
        features_per_node_switch_(RF_SQRT),
        bootstrap_sampling_(true),
        resample_count_(0),
        split_(RF_GINI),
        max_depth_(0),
        node_complexity_tau_(-1),
        min_num_instances_(1)
    {}

    RandomForestNewOptions & tree_count(int p_tree_count)
    {
        tree_count_ = p_tree_count;
        return *this;
    }

    RandomForestNewOptions & features_per_node(int p_features_per_node)
    {
        features_per_node_switch_ = RF_CONST;
        features_per_node_ = p_features_per_node;
        return *this;
    }

    RandomForestNewOptions & features_per_node(RandomForestOptionTags p_features_per_node_switch)
    {
        vigra_precondition(p_features_per_node_switch == RF_SQRT ||
                           p_features_per_node_switch == RF_LOG ||
                           p_features_per_node_switch == RF_ALL,
                           "RandomForestNewOptions::features_per_node(): Input must be RF_SQRT, RF_LOG or RF_ALL.");
        features_per_node_switch_ = p_features_per_node_switch;
        return *this;
    }

    RandomForestNewOptions & bootstrap_sampling(bool b)
    {
        bootstrap_sampling_ = b;
        return *this;
    }

    RandomForestNewOptions & resample_count(int n)
    {
        resample_count_ = n;
        bootstrap_sampling_ = false;
        return *this;
    }

    RandomForestNewOptions & split(RandomForestOptionTags p_split)
    {
        vigra_precondition(p_split == RF_GINI ||
                           p_split == RF_ENTROPY ||
                           p_split == RF_KSD,
                           "RandomForestNewOptions::split(): Input must be RF_GINI, RF_ENTROPY or RF_KSD.");
        split_ = p_split;
        return *this;
    }

    RandomForestNewOptions & max_depth(size_t d)
    {
        max_depth_ = d;
        return *this;
    }

    RandomForestNewOptions & node_complexity_tau(double tau)
    {
        node_complexity_tau_ = tau;
        return *this;
    }

    RandomForestNewOptions & min_num_instances(size_t n)
    {
        min_num_instances_ = n;
        return *this;
    }

    size_t get_features_per_node(size_t total) const
    {
        if (features_per_node_switch_ == RF_SQRT)
            return std::ceil(std::sqrt(total));
        else if (features_per_node_switch_ == RF_LOG)
            return std::ceil(std::log(total));
        else if (features_per_node_switch_ == RF_CONST)
            return features_per_node_;
        else if (features_per_node_switch_ == RF_ALL)
            return total;
        vigra_fail("RandomForestNewOptions::get_features_per_node(): Unknown switch.");
        return 0;
    }

    int tree_count_;
    int features_per_node_;
    RandomForestOptionTags features_per_node_switch_;
    bool bootstrap_sampling_;
    int resample_count_;
    RandomForestOptionTags split_;
    size_t max_depth_;
    double node_complexity_tau_;
    size_t min_num_instances_;

};



} // namespace vigra

#endif

