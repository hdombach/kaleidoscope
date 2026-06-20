#pragma once

#include "Object.hpp"

namespace serial {
	class Document {
		public:

			void start_transaction();
			void end_transaction();

		protected:
			friend class Object;

			void _implicit_start();
			void _implicit_end();

			void _ignore_start();
			void _ignore_end();

			void _add_transaction(Transaction::Ptr &&t);
	};
}
