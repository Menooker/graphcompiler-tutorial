#include <iostream>
#include <compiler/ir/graph/graph.hpp>
#include <compiler/ir/graph/driver.hpp>
#include <compiler/ir/graph/visitor.hpp>
#include <compiler/ir/graph/pass/pass.hpp>
#include <compiler/ir/builder.hpp>
#include <compiler/ir/graph/fusible_op.hpp>

using namespace sc;

sc_graph_t make_graph(sc_op_ptr &in, sc_op_ptr &matmul, sc_op_ptr &add, sc_op_ptr &out)
{
    sc_graph_t g;
    in = g.make_input({graph_tensor::make({1024, 1024}, sc_data_format_t::MK(), datatypes::f32),
                       graph_tensor::make({1024, 1024}, sc_data_format_t::KN(), datatypes::f32)});
    matmul = g.make("matmul_core", in->get_outputs(), {}, {});
    add = g.make("add", {matmul->get_outputs()[0], in->get_outputs()[1]}, {}, {});
    out = g.make_output(add->get_outputs());
    return g;
}

ir_module_ptr simple_lowering(context_ptr ctx, sc_graph_t &graph)
{
    std::unordered_map<graph_tensor_ptr, expr> ltsr_rtsr;
    int tensor_counter = 0;
    std::vector<expr> params;
    stmts func_body = make_stmt<stmts_node_t>(std::vector<stmt>());
    auto func = builder::make_func(
        "main_entry",
        params, func_body, datatypes::void_t);
    auto ret_mod = ir_module_t::from_entry_func(ctx, func);
    auto get_or_create_tensor = [&](const graph_tensor_ptr &t, bool is_arg) -> expr {
        auto itr = ltsr_rtsr.find(t);
        if (itr != ltsr_rtsr.end())
        {
            return itr->second;
        }
        if (!is_arg)
        {
            for (auto &use : t->uses_)
            {
                // finds if any of the use of the tensor is marked output
                if (use.second->isa<output_op>())
                {
                    is_arg = true;
                    break;
                }
            }
        }

        std::vector<expr> dims = dims_to_expr(t->details_.get_blocking_dims());
        std::string tensor_name = tensor_name = std::string("buffer_") + std::to_string(tensor_counter);
        expr tsr = builder::make_tensor(
            tensor_name, dims, t->details_.dtype_);
        tensor_counter++;
        ltsr_rtsr.insert(std::make_pair(t, tsr));

        if (!is_arg)
        {
            func_body->seq_.emplace_back(
                builder::make_var_tensor_def_unattached(tsr));
        }
        return tsr;
    };
    op_visitor_t vis = op_visitor_t::dfs_topology_sort();
    vis.visit_graph(graph, [&](const sc_op_ptr &node) {
        std::vector<expr> ins;
        std::vector<expr> outs;
        // special kinds of Ops that we need to take care of
        enum op_kinds
        {
            other = 0,
            input,
            output,
        } kind = other;
        if (node->isa<input_op>())
        {
            kind = input;
        }
        else if (node->isa<output_op>())
        {
            kind = output;
        }

        for (auto &ltensor : node->get_inputs())
        {
            ins.emplace_back(get_or_create_tensor(ltensor, false));
        }
        for (auto &ltensor : node->get_outputs())
        {
            outs.emplace_back(get_or_create_tensor(
                ltensor, kind == input));
        }
        switch (kind)
        {
        case input:
        {
            for (auto &v : outs)
            {
                params.emplace_back(v);
            }
            break;
        }
        case output:
        {
            for (auto &v : ins)
            {
                params.emplace_back(v);
            }
            break;
        }
        default:
        {
            std::vector<expr> exprargs;
            exprargs.insert(exprargs.end(), outs.begin(), outs.end());
            exprargs.insert(exprargs.end(), ins.begin(), ins.end());
            auto mod = node->get_func(ctx);
            ret_mod->merge(*mod);
            auto callee = mod->get_entry_func();
            stmts_node_t *target_body = func_body.get();
            target_body->seq_.emplace_back(
                builder::make_evaluate_unattached(
                    builder::make_call(callee, exprargs)));
        }
        }
    });

    func->params_ = std::move(params);
    func->decl_->params_ = func->params_;

    return ret_mod;
}

int dummy()
{
    sc_graph_t g;
    graph_driver(g);
}

int main()
{
    {
        auto ctx = get_default_context();
        sc_op_ptr in, matmul, add, out;
        sc_graph_t graph = make_graph(in, matmul, add, out);
        print_graph(graph, std::cout, true);

        expr in0 = builder::make_tensor("in0", {1024, 1024}, datatypes::f32);
        expr in1 = builder::make_tensor("in1", {1024, 1024}, datatypes::f32);
        expr out0 = builder::make_tensor("out", {1024, 1024}, datatypes::f32);
        stmts func_body = make_stmt<stmts_node_t>(std::vector<stmt>());
        func_t func = builder::make_func("main_entry", {in0, in1, out0}, stmt(func_body), datatypes::void_t);
        ir_module_ptr mod = ir_module_t::from_entry_func(ctx, func);

        ir_module_ptr matmul_mod = matmul->get_func(ctx);
        func_t matmul_func = matmul_mod->get_entry_func();
        mod->merge(*matmul_mod);
        expr mm_out = builder::make_tensor("matmul_out", {1024, 1024}, datatypes::f32);
        func_body->seq_.emplace_back(builder::make_var_tensor_def_unattached(mm_out));
        func_body->seq_.emplace_back(builder::make_evaluate_unattached(builder::make_call(matmul_func, {mm_out, in0, in1})));

        ir_module_ptr add_mod = add->get_func(ctx);
        func_t add_func = add_mod->get_entry_func();
        mod->merge(*add_mod);
        func_body->seq_.emplace_back(builder::make_evaluate_unattached(builder::make_call(add_func, {out0, mm_out, in1})));

        std::cout << mod;
    }
}