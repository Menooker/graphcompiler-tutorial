#include <iostream>
#include <compiler/ir/sc_function.hpp>
#include <compiler/ir/builder.hpp>
#include <compiler/ir/easy_build.hpp>

using namespace sc;

void example1()
{
    auto v_stmts = make_stmt<stmts_node_t>(std::vector<stmt>());
    auto var_a = make_expr<var_node>(datatypes::s32, "a");
    v_stmts->seq_.emplace_back(make_stmt<define_node_t>(var_a, linkage::local, expr()));
    auto v_1_2 = make_expr<add_node>(make_expr<constant_node>(int64_t(1)), make_expr<constant_node>(int64_t(2)));
    v_stmts->seq_.emplace_back(make_stmt<assign_node_t>(var_a, v_1_2));

    std::cout << v_stmts << std::endl;
}

void example2()
{
    using namespace sc::builder;
    auto v_stmts = make_stmts_unattached({}).checked_as<stmts>();
    auto var_a = make_var(datatypes::s32, "a");
    v_stmts->seq_.emplace_back(make_var_tensor_def_unattached(var_a, linkage::local, expr()));
    auto v_1_2 = make_add(make_expr<constant_node>(int64_t(1)), make_expr<constant_node>(int64_t(2)));
    v_stmts->seq_.emplace_back(make_assign_unattached(var_a, v_1_2));

    std::cout << v_stmts << std::endl;
}

void example3()
{
    builder::ir_builder_t bld;
    _function_(datatypes::f32, some_func, _arg_("idx", datatypes::s32), _arg_("A", datatypes::f32, {100, 200}))
    {
        _bind_(idx, A);
        _var_(b, datatypes::f32);
        b = 0;
        _for_(i, 0, 200)
        {
            b = b + A[{idx, i}];
        }
        _return_(b);
    }

    std::cout << some_func;
}

int main()
{
    example1();
    example2();
    example3();
}