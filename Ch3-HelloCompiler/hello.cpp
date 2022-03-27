#include <iostream>
#include <compiler/ir/graph/graph.hpp>
#include <compiler/ir/graph/driver.hpp>
#include <compiler/ir/graph/pass/pass.hpp>
#include <compiler/ir/graph/lowering.hpp>
#include <compiler/jit/jit.hpp>

using namespace sc;

int main()
{
    auto ctx = get_default_context();
    sc_graph_t g;
    auto in = g.make_input({graph_tensor::make({1024, 1024}, sc_data_format_t(), datatypes::f32),
                            graph_tensor::make({1024, 1024}, sc_data_format_t(), datatypes::f32)});
    //auto mm1 = g.make("matmul_core", in->get_outputs(), {}, {});
    //auto relu = g.make("relu", mm1->get_outputs(), {}, {});
    auto relu = g.make("relu", {in->get_outputs()[0]}, {}, {});
    auto add = g.make("add", {in->get_outputs()[0], in->get_outputs()[1]}, {}, {});
    auto out = g.make_output(add->get_outputs());
    std::cout << "Original Graph:\n";
    print_graph(g, std::cout, true);
    graph_driver(g, ctx);

    std::cout << "Graph After passes:\n";
    print_graph(g, std::cout, true);

    auto ir_modu = lower_graph(ctx, g, {in, out});
    std::cout << "Tensor IR:\n"
              << ir_modu << '\n';

    auto jit_func = jit_engine_t::make(ctx)->get_entry_func(ir_modu);
    std::vector<float> a(1024 * 1024), b(1024 * 1024), outbuffer(1024 * 1024);
    jit_func->call_default<void>(a.data(), b.data(), outbuffer.data());
}