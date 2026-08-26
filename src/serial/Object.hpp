#pragma once

#include <stdint.h>
#include <memory>
#include <vector>

namespace serial {
	class Document;
	class Object;

	/**
	 * @brief Represents a single change that you can undo or redo in the document
	 *
	 * Internally, a single transaction might be multiple changes. However, there
	 * is always only one transaction per undo or redo.
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
			CompoundTransaction() = default;

			static Ptr create();

			static Ptr create(std::vector<Ptr> &&t);

			~CompoundTransaction() = default;

			void add_transaction(Ptr &&t);

			Transaction::Ptr apply(Object &obj) override;
		private:
			std::vector<Ptr> _children;
	};

	/**
	 * @brief A Generic abstract class for objects that can be serialized
	 */
	class Object {
		public:
			virtual ~Object() = default;

			/**
			 * @brief A unique identifier for the types.
			 *
			 * Is gauranteed to be different for all types that are defined in a ".dt"
			 * along with includes
			 */
			virtual uint32_t type_id() const = 0;
			/**
			 * @brief A string representation for the type
			 */
			virtual const char *type_str() const = 0;

			/**
			 * @brief Starts recording a transaction
			 *
			 * Can be used if you manually want to make multiple changes in a single
			 * transaction.
			 *
			 * You should not call start_transaction multiple times without call
			 * end_transaction after each one. (You should not nest calls)
			 */
			virtual void start_transaction();

			/**
			 * @brief Stops recording a transaction
			 */
			virtual void end_transaction();

		protected:
			/**
			 * @brief Automatically start a transaction when you setting a field
			 *
			 * This class is meant to be called multiple times in a row.
			 * (Objects can nest transaction calls since we can ensure it is unwound
			 * completely)
			 */
			virtual void _implicit_start();
			/**
			 * @brief Automatically stops a transaction after a field is set
			 */
			virtual void _implicit_end();

			/**
			 * @brief Marks that a transaction should not be implicitly started
			 *
			 * This is used when applying a transaction during an undo or redo
			 */
			virtual void _ignore_start();
			/**
			 * @brief Marks the end of the implicit region
			 */
			virtual void _ignore_end();

			/**
			 * @brief Add a transaction.
			 *
			 * The transaction is passed up to the root document class.
			 * Each step along the way, it is recorded which field exaclty is modified.
			 * That way, the correct child can be found even after it changes
			 * location in memory.
			 */
			virtual void _add_transaction(Transaction::Ptr &&t);

			void _adopt_child(Object *child);

			void _set_idx(Object *child, uint32_t idx);

		protected:
			Object *_parent = nullptr;
			uint32_t _idx = 0;
	};

	class Document : public Object {
		public:
			virtual ~Document() = default;

			uint32_t type_id() const override = 0;
			const char *type_str() const override = 0;

			void start_transaction() override;
			void end_transaction() override;

		protected:
			void _implicit_start() override;
			void _implicit_end() override;

			void _ignore_start() override;
			void _ignore_end() override;

			void _add_transaction(Transaction::Ptr &&t) override = 0;
			/**
			 * @brief Add the transaction as the root document
			 *
			 * Adds the transaction to a list of pending transactions which is then
			 * added to the final list once a transaction is finished.
			 */
			void _add_transaction_final(Transaction::Ptr &&t);

		private:
			int _recording_t = 0;
			int _ignoring_t = 0;

			// Can either be null, a standalone transaction, or a compound transaction
			Transaction::Ptr _pending_transactions;
	};
}
