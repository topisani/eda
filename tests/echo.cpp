#include "eda/evaluator.hpp"
#include "eda/syntax.hpp"

static auto filter()
{
  using namespace eda;
  using namespace eda::syntax;

  //ABlock<2, 1> auto const filter = (_ << (_, _), _) | (((_ * _, (1 - _) * _) | plus) % _);
  ABlock<2, 1> auto const filter = _ * _;
  return make_evaluator(filter);
}

struct Filter {
  void process(const float* in, float* out, float a, unsigned nframes);

private:
  decltype(filter()) evaluator = filter();
};

void Filter::process(const float* in, float* out, float a, unsigned nframes)
{
  for (int i = 0; i < nframes; i++) {
    out[i] = evaluator.eval({a, in[i]});
  }
}
