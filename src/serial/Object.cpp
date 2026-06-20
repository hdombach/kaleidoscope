#include "Object.hpp"
#include "Document.hpp"

namespace serial {
	CompoundTransaction::CompoundTransaction(std::vector<Ptr> &&t): _children(std::move(t)) {}

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
		return Ptr(new CompoundTransaction(std::move(result)));
	}

	void Object::start_transaction() { _doc->start_transaction(); }
	void Object::end_transaction() { _doc->end_transaction(); }

	void Object::_implicit_start() { _doc->_implicit_start(); }
	void Object::_implicit_end() { _doc->_implicit_end(); }

	void Object::_ignore_start() { _doc->_ignore_start(); }
	void Object::_ignore_end() { _doc->_ignore_end(); }

	void Object::_add_transaction(Transaction::Ptr &&t) {
		_doc->_add_transaction(std::move(t));
	}
}
