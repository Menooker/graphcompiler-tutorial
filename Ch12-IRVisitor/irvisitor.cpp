#include <iostream>
#include <compiler/ir/easy_build.hpp>
#include <compiler/ir/visitor.hpp>

using namespace sc;

class simple_constant_folder : public ir_visitor_t
{
    expr_c visit(add_c v) override
    {
        auto lhs = dispatch(v->l_);
        auto rhs = dispatch(v->r_);

        if (lhs.isa<constant>() && rhs.isa<constant>() && lhs->dtype_ == datatypes::s32)
        {
            int64_t lval = lhs.static_as<constant>()->value_[0].s64;
            int64_t rval = rhs.static_as<constant>()->value_[0].s64;
            return make_expr<constant_node>(lval + rval, datatypes::s32);
        }
        bool changed = !lhs.ptr_same(v->l_) || !rhs.ptr_same(v->r_);
        if (changed)
        {
            return builder::make_add(lhs, rhs);
        }
        return v;
    }

    expr_c visit(sub_c v) override
    {
        auto lhs = dispatch(v->l_);
        auto rhs = dispatch(v->r_);

        if (lhs.isa<constant>() && rhs.isa<constant>() && lhs->dtype_ == datatypes::s32)
        {
            int64_t lval = lhs.static_as<constant>()->value_[0].s64;
            int64_t rval = rhs.static_as<constant>()->value_[0].s64;
            return make_expr<constant_node>(lval - rval, datatypes::s32);
        }
        bool changed = !lhs.ptr_same(v->l_) || rhs.ptr_same(v->r_);
        if (changed)
        {
            return builder::make_sub(lhs, rhs);
        }
        return v;
    }
};

int main()
{
    expr a = expr(1) + expr(2) - expr(3);
    expr b = expr(1) + expr(2) + builder::make_var(datatypes::s32, "var_b");
    simple_constant_folder folder;
    std::cout << "expr a before folder:" << a << ", after folder:" << folder.dispatch(a) << '\n';
    std::cout << "expr b before folder:" << b << ", after folder:" << folder.dispatch(b) << '\n';
}