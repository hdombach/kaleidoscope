#pragma once

#include "Object.hpp"

namespace serial {
	class Document {
		public:
			virtual ~Document() = default;

			virtual uint32_t type_id() const = 0;
			virtual const char *type_str() const = 0;

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
