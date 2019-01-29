
#include "run_sp.h"

#include "gtfs_graph.h"

template <typename T>
void inst_graph (std::shared_ptr<GTFSGraph> g, unsigned int nedges,
        const std::map <std::string, unsigned int>& vert_map,
        const std::vector <std::string>& from,
        const std::vector <std::string>& to,
        const std::vector <T>& dist,
        const std::vector <T>& transfer)
{
    for (unsigned int i = 0; i < nedges; ++i)
    {
        unsigned int fromi = vert_map.at(from [i]);
        unsigned int toi = vert_map.at(to [i]);
        g->addNewEdge (fromi, toi, dist [i], transfer [i]);
    }
}

size_t run_sp::make_vert_map (const Rcpp::DataFrame &vert_map_in,
        const std::vector <std::string> &vert_map_id,
        const std::vector <unsigned int> &vert_map_n,
        std::map <std::string, unsigned int> &vert_map)
{
    for (unsigned int i = 0;
            i < static_cast <unsigned int> (vert_map_in.nrow ()); ++i)
    {
        vert_map.emplace (vert_map_id [i], vert_map_n [i]);
    }
    size_t nverts = static_cast <size_t> (vert_map.size ());
    return (nverts);
}

//' rcpp_get_sp_dists
//'
//' @noRd
// [[Rcpp::export]]
Rcpp::NumericMatrix rcpp_get_sp_dists (const Rcpp::DataFrame graph,
        const Rcpp::DataFrame vert_map_in,
        Rcpp::IntegerVector fromi,
        Rcpp::IntegerVector toi)
{
    size_t nfrom = static_cast <size_t> (fromi.size ());
    size_t nto = static_cast <size_t> (toi.size ());

    std::vector <std::string> from = graph ["from"];
    std::vector <std::string> to = graph ["to"];
    std::vector <int> dist = graph ["d"];
    std::vector <int> transfer = graph ["transfer"];

    unsigned int nedges = static_cast <unsigned int> (graph.nrow ());
    std::map <std::string, unsigned int> vert_map;
    std::vector <std::string> vert_map_id = vert_map_in ["vert"];
    std::vector <unsigned int> vert_map_n = vert_map_in ["id"];
    size_t nverts = run_sp::make_vert_map (vert_map_in, vert_map_id,
            vert_map_n, vert_map);

    std::shared_ptr <GTFSGraph> g = std::make_shared <GTFSGraph> (nverts);
    inst_graph (g, nedges, vert_map, from, to, dist, transfer);

    std::shared_ptr <Dijkstra> dijkstra =
        std::make_shared <Dijkstra> (nverts, g);

    std::vector <int> d (nverts);
    std::vector <int> prev (nverts);

    dijkstra->init (g); // specify the graph

    // initialise dout matrix to NA
    Rcpp::NumericVector na_vec = Rcpp::NumericVector (nfrom * nto,
            Rcpp::NumericVector::get_na ());
    Rcpp::NumericMatrix dout (static_cast <int> (nfrom),
            static_cast <int> (nto), na_vec.begin ());

    for (unsigned int v = 0; v < nfrom; v++)
    {
        Rcpp::checkUserInterrupt ();
        std::fill (d.begin(), d.end(), INFINITE_INT);

        dijkstra->run (d, prev, static_cast <unsigned int> (fromi [v]));
        for (unsigned int vi = 0; vi < nto; vi++)
        {
            if (d [static_cast <size_t> (toi [vi])] < INFINITE_INT)
            {
                dout (v, vi) = d [static_cast <size_t> (toi [vi])];
            }
        }
    }
    return (dout);
}


//' rcpp_get_paths
//'
//' @param graph The data.frame holding the graph edges
//' @param vert_map_in map from <std::string> vertex ID to (0-indexed) integer
//' index of vertices
//' @param fromi Index into vert_map_in of vertex numbers
//' @param toi Index into vert_map_in of vertex numbers
//'
//' @note The graph is constructed with 0-indexed vertex numbers contained in
//' code{vert_map_in}. Both \code{fromi} and \code{toi} already map directly
//' onto these. The graph has to be constructed by first constructing a
//' \code{std::map} object (\code{vertmap}) for \code{vert_map_in}, then
//' translating all \code{graph["from"/"to"]} values into these indices. This
//' construction is done in \code{inst_graph}.
//'
//' @noRd
// [[Rcpp::export]]
Rcpp::List rcpp_get_paths (const Rcpp::DataFrame graph,
        const Rcpp::DataFrame vert_map_in,
        Rcpp::IntegerVector fromi,
        Rcpp::IntegerVector toi)
{
    Rcpp::NumericVector id_vec;
    size_t nfrom = static_cast <size_t> (fromi.size ());
    size_t nto = static_cast <size_t> (toi.size ());

    std::vector <std::string> from = graph ["from"];
    std::vector <std::string> to = graph ["to"];
    std::vector <int> dist = graph ["d"];
    std::vector <int> transfer = graph ["transfer"];

    unsigned int nedges = static_cast <unsigned int> (graph.nrow ());
    std::map <std::string, unsigned int> vert_map;
    std::vector <std::string> vert_map_id = vert_map_in ["vert"];
    std::vector <unsigned int> vert_map_n = vert_map_in ["id"];
    size_t nverts = run_sp::make_vert_map (vert_map_in, vert_map_id,
            vert_map_n, vert_map);

    std::shared_ptr <GTFSGraph> g = std::make_shared <GTFSGraph> (nverts);
    inst_graph (g, nedges, vert_map, from, to, dist, transfer);

    std::shared_ptr<Dijkstra> dijkstra =
        std::make_shared <Dijkstra> (nverts, g);
    
    Rcpp::List res (nfrom);
    std::vector <int> d(nverts);
    std::vector <int> prev(nverts);

    dijkstra->init (g); // specify the graph

    for (unsigned int v = 0; v < nfrom; v++)
    {
        Rcpp::checkUserInterrupt ();
        std::fill (d.begin(), d.end(), INFINITE_INT);

        dijkstra->run (d, prev, static_cast <unsigned int> (fromi [v]));

        Rcpp::List res1 (nto);
        for (unsigned int vi = 0; vi < nto; vi++)
        {
            std::vector <unsigned int> onePath;
            if (d [static_cast <size_t> (toi [vi])] < INFINITE_INT)
            {
                int target = toi [vi]; // target can be -1!
                while (target < INFINITE_INT)
                {
                    // Note that targets are all C++ 0-indexed and are converted
                    // directly here to R-style 1-indexes.
                    onePath.push_back (static_cast <unsigned int> (target + 1));
                    target = prev [static_cast <size_t> (target)];
                    if (target < 0 || target == fromi [v])
                        break;
                }
            }
            std::reverse (onePath.begin (), onePath.end ());
            if (onePath.size () > 1)
                res1 [vi] = onePath;
        }
        res [v] = res1;
    }
    return (res);
}