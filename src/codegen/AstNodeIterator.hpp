#include "AstNode.hpp"

namespace cg {
	class AstNode;
	class AstNodeIterator {
		public:
			using reference = AstNode&;
			using const_reference = const AstNode&;
			
			using pointer = AstNode*;
			using const_pointer = const AstNode*;

			AstNodeIterator(AstNode *node): _node(node) {}

			AstNodeIterator& operator++() {
				_node = _node->_sibling_next;
				return *this;
			}

			AstNodeIterator operator++(int) {
				auto r = *this;
				++(*this);
				return r;
			}

			reference operator[](uint32_t i) {
				auto r = *this;
				while (i > 0) {
					r++;
					i--;
				}
				return *r;
			}

			bool operator==(AstNodeIterator const &other) const {
				return _node == other._node;
			}
			bool operator!=(AstNodeIterator const &other) const {
				return !(*this == other);
			}

			reference operator*() { return *_node; }
			const_reference operator*() const { return *_node; }

			pointer operator->() { return _node; }
			const_pointer operator->() const { return _node; }


		private:
			AstNode *_node=nullptr;;
	};
};
