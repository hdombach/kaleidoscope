#pragma once

namespace util {
	template<typename I>
		class IterAdapter {
			public:
				IterAdapter(I begin, I end): _begin(begin), _end(end) {}
				I begin() { return _begin; }
				I end() { return _end; }
			private:
				I _begin;
				I _end;
		};

	#define Adapt(BEGIN, END) IterAdapter<typeof(BEGIN)>(BEGIN, END)
}
