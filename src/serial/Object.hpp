#pragma once

#include <stdint.h>
#include <memory>
#include <vector>

namespace serial {
	class Document;
	class Object;

	/**
	 * @brief Keeps track of changes made to the document
	 * Allows you to undo and redo
	 */
	class Transaction {
		public:
			using Ptr = std::unique_ptr<Transaction>;

			virtual ~Transaction() = default;

			/**
			 * @brief Applies the change in data
			 * @returns The reverse transaction
			 */
			virtual Ptr apply(Object &obj) = 0;
	};

	class CompoundTransaction: public Transaction {
		public:
			CompoundTransaction(std::vector<Ptr> &&t);

			~CompoundTransaction() = default;

			void add_transaction(Ptr &&t);

			Ptr apply(Object &obj) override;
		private:
			std::vector<Ptr> _children;
	};

	class Object {
		public:
			virtual ~Object() = default;

			virtual uint32_t type_id() const = 0;
			virtual const char *type_str() const = 0;

			void start_transaction();
			void end_transaction();

		protected:
			void _implicit_start();
			void _implicit_end();

			void _ignore_start();
			void _ignore_end();

			void _add_transaction(Transaction::Ptr &&t);

		protected:
			Document *_doc = nullptr;
	};

}
