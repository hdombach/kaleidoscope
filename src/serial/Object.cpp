#include "Object.hpp"
#include "util/log.hpp"

namespace serial {
	CompoundTransaction::Ptr CompoundTransaction::create() {
		return std::make_unique<CompoundTransaction>();
	}

	CompoundTransaction::Ptr CompoundTransaction::create(std::vector<Ptr> &&t) {
		auto ptr = std::make_unique<CompoundTransaction>();
		ptr->_children = std::move(t);
		return ptr;
	}

	void CompoundTransaction::add_transaction(Ptr &&t) {
		_children.push_back(std::move(t));
	}

	Transaction::Ptr CompoundTransaction::apply(Object &obj) {
		auto result = std::vector<Ptr>(_children.size());
		int i = _children.size() - 1;
		for (auto &child : _children) {
			result[i] = child->apply(obj);
			i--;
		}
		return CompoundTransaction::create(std::move(result));
	}

	void Object::start_transaction() {
		if (_parent)
			_parent->start_transaction();
	}
	void Object::end_transaction() {
		if (_parent)
			_parent->end_transaction();
	}

	void Object::_implicit_start() {
		if (_parent)
			_parent->_implicit_start();
	}
	void Object::_implicit_end() {
		if (_parent)
			_parent->_implicit_end();
	}

	void Object::_ignore_start() {
		if (_parent)
			_parent->_ignore_start();
	}
	void Object::_ignore_end() {
		if (_parent)
			_parent->_ignore_end();
	}

	void Object::_add_transaction(Transaction::Ptr &&t) {
		if (_parent)
			_parent->_add_transaction(std::move(t));
	}

	void Object::_update_parent(Object *child, Object *parent) {
		child->_parent = parent;
	}

	void Document::start_transaction() {
		if (_ignoring_t) {
			log_error() << "You somehow started a transaction in the middle of calling undo or redo" << std::endl;
		}
		if (_recording_t) {
			log_warning() << "Cannot start multiple serial transactions at the same time." << std::endl;
		}
		_implicit_start();
	}

	void Document::end_transaction() {
		if (_ignoring_t) {
			log_error() << "You somehow are stopping a transaction in the middle of calling undo or redo" << std::endl;
		}
		if (!_recording_t) {
			log_warning() << "Cannot stop a serial transaction if not none are running." << std::endl;
			_implicit_end();
		}
	}

	void Document::_implicit_start() {
		if (_ignoring_t) return;
		_recording_t++;
	}

	void Document::_implicit_end() {
		if (_ignoring_t) return;
		if (_recording_t <= 0) {
			log_error() << "Cannot properly keep track of when a transaction should start or end anymore" << std::endl;
			return;
		}

		_recording_t--;
	}

	void Document::_ignore_start() {
		_ignoring_t++;
	}

	void Document::_ignore_end() {
		if (_ignoring_t <= 0) {
			log_error() << "The ignoring state got out of synch" << std::endl;
			return;
		}

		_ignoring_t--;
	}

	void Document::_add_transaction_final(Transaction::Ptr &&t) {
		// Can either be null, a standalone transaction, or a compound.
		//
		if (_pending_transactions == nullptr) {
			// No transaction currently being recorded
			_pending_transactions = std::move(t);
		} else if (auto compound = dynamic_cast<CompoundTransaction*>(_pending_transactions.get())) {
			compound->add_transaction(std::move(t));
		} else {
			// Transition from single transaction to a compound transaction
			auto temp = std::move(_pending_transactions);
			_pending_transactions = CompoundTransaction::create();
			auto &c = static_cast<CompoundTransaction&>(*_pending_transactions);
			c.add_transaction(std::move(temp));
			c.add_transaction(std::move(t));
		}
	}
}
