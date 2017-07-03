#include <vector>

#include "caffe/layers/affinity_loss_layer.hpp"
#include "caffe/util/math_functions.hpp"

namespace caffe {

template<typename Dtype>
void AffinityLossLayer<Dtype>::Reshape(const vector<Blob<Dtype>*>& bottom,
    const vector<Blob<Dtype>*>& top) {
  LossLayer<Dtype>::Reshape(bottom, top);
  CHECK_EQ(bottom[0]->count(1), bottom[1]->count(1))<< "Inputs must have the same dimension.";
  diff_.ReshapeLike(*bottom[0]);
}

template<typename Dtype>
void AffinityLossLayer<Dtype>::Forward_cpu(const vector<Blob<Dtype>*>& bottom,
    const vector<Blob<Dtype>*>& top) {
  int count = bottom[0]->count();
  Dtype sum = 0.0;
  Dtype gap = this->layer_param_.affinity_loss_param().gap() / 2.0;
  Dtype scale = this->layer_param_.affinity_loss_param().scale();
  bool huber = this->layer_param_.affinity_loss_param().pseudohuber();

  Dtype scale2 = scale*scale;
  const Dtype *labels = bottom[1]->cpu_data();
  const Dtype *preds = bottom[0]->cpu_data();
  Dtype *d = diff_.mutable_cpu_data();

  for (unsigned i = 0; i < count; i++) {
    Dtype label = labels[i];
    Dtype pred = preds[i];
    Dtype diff = 0.0;
    if (label > 0) { //normal euclidean
      diff = pred - label;
      if (diff < 0) {
        diff = std::min(diff + gap, Dtype(0));
      } else {
        diff = std::max(diff - gap, Dtype(0));
      }
    } else if (label < 0 && pred > -label) { //hinge like
      diff = pred + label;
      if (diff < 0) {
        diff = std::min(diff + gap, Dtype(0));
      } else {
        diff = std::max(diff - gap, Dtype(0));
      }
    } else { //ignore
      diff = 0;
    }

    d[i] = diff;

    if(huber) {
      //https://en.wikipedia.org/wiki/Huber_loss
      Dtype hval = diff/scale;
      sum += scale2*(sqrt(1+hval*hval) - 1.0);
    }
    else {
      sum += diff * diff;
    }

  }

  Dtype loss = sum / bottom[0]->num() / Dtype(2);
  top[0]->mutable_cpu_data()[0] = loss;
}

template<typename Dtype>
void AffinityLossLayer<Dtype>::Backward_cpu(const vector<Blob<Dtype>*>& top,
    const vector<bool>& propagate_down, const vector<Blob<Dtype>*>& bottom) {

  bool huber = this->layer_param_.affinity_loss_param().pseudohuber();
  Dtype scale = this->layer_param_.affinity_loss_param().scale();

  for (int i = 0; i < 2; ++i) {
    if (propagate_down[i]) {

      if(huber) {
        //x/(1+(x/scale)^2)
        const Dtype *diff = diff_.cpu_data();
        Dtype *out = bottom[i]->mutable_cpu_diff();
        for(unsigned j = 0, n = bottom[i]->count(); j < n; j++) {
          Dtype x = diff[j];
          Dtype val = x/scale;
          out[j] = x/(1.0 + val*val);
        }
      } else {
        const Dtype sign = (i == 0) ? 1 : -1;
        const Dtype alpha = sign * scale * top[0]->cpu_diff()[0] / bottom[i]->num();
        caffe_cpu_axpby(bottom[i]->count(),              // count
            alpha,                              // alpha
            diff_.cpu_data(),                   // a
            Dtype(0),                           // beta
            bottom[i]->mutable_cpu_diff());  // b
      }
    }
  }
  /*LOG(INFO) << "AFFGRADS";
   for(unsigned i = 0, n = bottom[0]->num(); i < n; i++) {
   LOG(INFO) << bottom[0]->cpu_diff()[i];
   }*/
}

INSTANTIATE_CLASS(AffinityLossLayer);
REGISTER_LAYER_CLASS(AffinityLoss);

}  // namespace caffe
