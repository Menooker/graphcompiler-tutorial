#include <iostream>
#include <compiler/ir/graph/graph.hpp>
#include <compiler/ir/graph/driver.hpp>
#include <compiler/ir/graph/pass/pass.hpp>

using namespace sc;

sc_graph_t make_graph(sc_op_ptr &in, sc_op_ptr &relu, sc_op_ptr &add, sc_op_ptr &out)
{
    sc_graph_t g;
    in = g.make_input({graph_tensor::make({1024, 1024}, sc_data_format_t(), datatypes::f32),
                       graph_tensor::make({1024, 1024}, sc_data_format_t(), datatypes::f32)});
    relu = g.make("relu", {in->get_outputs()[0]}, {}, {});
    add = g.make("add", {relu->get_outputs()[0], in->get_outputs()[1]}, {}, {});
    out = g.make_output(add->get_outputs());
    return g;
}

int dummy()
{
    sc_graph_t g;
    graph_driver(g);
}

int main()
{
    {
        sc_op_ptr in, relu, add, out;
        sc_graph_t graph = make_graph(in, relu, add, out);
        add->replace_input(1, in->get_outputs()[0]);
        print_graph(graph, std::cout, true);
        in->get_outputs()[0]->replace_with(in->get_outputs()[1]);
        print_graph(graph, std::cout, true);
    }

    {
        sc_op_ptr in, relu, add, out;
        sc_graph_t graph = make_graph(in, relu, add, out);
        auto sub = graph.make("sub", add->get_inputs(), {}, {});
        add->replace_uses_with_and_remove(sub);
        print_graph(graph, std::cout, true);
    }
}