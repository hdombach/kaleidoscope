#include <ostream>

namespace util {
	class NullStream: public std::ostream {
		public:
			class NullBuffer : public std::streambuf {
				public:
					int overflow( int c ) { return c; }
			} m_nb;

		public:
			NullStream() : std::ostream( &m_nb ) {}
	};

	static inline auto null_stream = NullStream();
}
