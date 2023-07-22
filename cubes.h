#pragma once

#include <cstdlib>
#include <fstream>
#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <sstream>
#include <vector>
#include <unordered_set>


#include "polycube_sparse.h"
#include "stack_allocator.h"
#include "thread_safe_queue.h"

//////////////////////////////////////////////////
// Rooted method starts here
//////////////////////////////////////////////////

using output_t = size_t;


const int MAX_DIMENSIONS = 20;
//could be smaller - based on function optimization of ( 1 + x )* (1 + y )*( 1 + z) subject to x + y + z = MAX_DIMENSIONS with x,y,z > 0 -> max elements comes out to ~ (MAX_DIMENSIONS / 3 + 1)^3
const int MAX_ENTRIES = MAX_DIMENSIONS * MAX_DIMENSIONS * MAX_DIMENSIONS;

const int16_t FILLED_CUBE = 0x7FFF;

/// <summary>
/// Rooted polycube, based on description given here: http://kevingong.com/Polyominoes/ParallelPoly.html
/// </summary>
struct rooted_polycube
{
    int k; //number of cubes
    position root; //position of root
    position dim; //dim of this cube
    int highest_numbering; //highest number cube filled in
    int highest_written; //highest value already used to mark

    uint16_t cubes[MAX_ENTRIES]; //elements must be able to store MAX_ENTRIES

    position min_bounds; //Minimum values of written cubes
    position max_bounds; //maximum values of written cubes

    position labeled_min_bounds; //Minimum value of written labels (unfilled numbered cubes);
    position labeled_max_bounds; //Maximum value of written labels (unfilled numbered cubes);

    struct
    {
        position stack[32]; //Filled Cubes Relative to root
        size_t current;
    } filled_cubes;

#ifdef _DEBUG
    std::vector<int> debug_push_order;
#endif

    size_t size()
    {
        return (size_t)dim.x * dim.y * dim.z;
    }

    int16_t get_cube(int x, int y, int z) const
    {
        int index = z * (dim.y * (int) dim.x) + y * dim.x + x;
        return cubes[index];
    }

    void set_cube(int x, int y, int z, int16_t elem)
    {
        int index = z * (dim.y * (int)dim.x) + y * dim.x + x;
        cubes[index] = elem;
    }
    void set_cube_if_zero_and_increment(int x, int y, int z,  int& inout_next_highest)
    {
        int index = z * (dim.y * (int) dim.x) + y * dim.x + x;
        if (cubes[index] == 0)
        {
            cubes[index] = inout_next_highest;
            inout_next_highest++;
        }
    }

    void set_cube_if_zero_and_increment_and_update_bounds(int x, int y, int z, int& inout_next_highest)
    {
        int index = z * (dim.y * (int)dim.x) + y * dim.x + x;
        if (cubes[index] == 0)
        {
            cubes[index] = inout_next_highest;
            inout_next_highest++;

            position current = { (int8_t) x, (int8_t)y, (int8_t)z };
            position_min(labeled_min_bounds, current);
            position_max(labeled_max_bounds, current);
        }
    }

    template<typename Func>
    void for_each_cube(Func&& func) const
    {
        int index = 0;
        for (int8_t z = 0; z < dim.z; z++)
        {
            for (int8_t y = 0; y < dim.y; y++)
            {
                for (int8_t x = 0; x < dim.x; x++)
                {
                    func(x, y, z, cubes[index]);
                    index++;
                }
            }
        }
    }

    /// <summary>
    /// Iterates through all filled cubes by full position, after offsetting from root
    /// </summary>
    /// <typeparam name="Func"></typeparam>
    /// <param name="func"></param>
    template<typename Func>
    void for_each_filled(Func&& func) const
    {
        for (size_t i = 0; i < filled_cubes.current; i++)
        {
            position cube = filled_cubes.stack[i] + root;
            func(cube.x, cube.y, cube.z, i);
        }
    }

    bool check_root() const
    {
        return get_cube(root.x, root.y, root.z) == FILLED_CUBE;
    }

};

//Assuming a max of 2 per stack frame, 32 depth
using stack_allocator = stack_allocator_typed<rooted_polycube, 64>;
using stack_marker = stack_marker_typed<rooted_polycube, 64>;

/// <summary>
/// pads a polycube with zeros to allow for expanding. Assumes expansion won't violate rooted property
/// </summary>
/// <param name="pc"></param>
/// <returns></returns>
inline void pad_cube(const rooted_polycube& base, rooted_polycube& out_padded, position& out_lower_delta)
{
    if (!base.check_root())
    {
        printf("WTF?");
    }

    //We should only need to pad in the direction of the most recently found cube

    position lower_delta = { 1,1,1 };
    position higher_delta = { 1,1,1 };

    if (base.k > 3)
    {
        position last = base.root + base.filled_cubes.stack[base.filled_cubes.current - 1];

        lower_delta = { 0,0,0 };
        higher_delta = { 0,0,0 };

        if (last.x == 0)
        {
            lower_delta.x = 1;
        }
        else if (last.y == 0)
        {
            lower_delta.y = 1;
        }
        else if (last.z == 0)
        {
            lower_delta.z = 1;
        }

        if (last.x == base.dim.x - 1)
        {
            higher_delta.x = 1;
        }
        else if (last.y == base.dim.y - 1)
        {
            higher_delta.y = 1;
        }
        else if (last.z == base.dim.z - 1)
        {
            higher_delta.z = 1;
        }
    }

    out_lower_delta = lower_delta;


    position delta = lower_delta + higher_delta;

    out_padded.k = base.k;
    out_padded.highest_numbering = base.highest_numbering;
    out_padded.highest_written = base.highest_written;
    out_padded.dim = base.dim + delta;
    out_padded.root = base.root + lower_delta;
    out_padded.min_bounds = base.min_bounds + lower_delta;
    out_padded.max_bounds = base.max_bounds + lower_delta;
    out_padded.labeled_min_bounds = base.labeled_min_bounds + lower_delta;
    out_padded.labeled_max_bounds = base.labeled_max_bounds + lower_delta;

    out_padded.filled_cubes = base.filled_cubes;

#ifdef _DEBUG
    out_padded.debug_push_order = base.debug_push_order;
#endif

    memset(out_padded.cubes, 0, out_padded.size() * sizeof(out_padded.cubes[0]));
    base.for_each_cube([&](int x, int y, int z, int element)
    {
        if (element)
        {
            out_padded.set_cube(x + lower_delta.x, y + lower_delta.y, z + lower_delta.z, element);
        }
    });

    if (!out_padded.check_root())
    {
        printf("WTF?");
    }
}

/// <summary>
/// Pads a cube, fills in unnumbered slots that can be filled
/// </summary>
/// <param name="pc"></param>
/// <returns></returns>
inline void expand_empty_slots(const rooted_polycube& base, rooted_polycube& out_expanded)
{
    if (!base.check_root())
    {
        printf("WTF?");
    }

    position lower_delta;
    pad_cube(base, out_expanded, lower_delta);

    int next_highest = base.highest_written + 1;

    //We only need to expand the most recently added cube
    position last = base.root + base.filled_cubes.stack[base.filled_cubes.current - 1];
    {
        int _x = last.x + lower_delta.x, _y = last.y + lower_delta.y, _z = last.z + lower_delta.z;
        out_expanded.set_cube_if_zero_and_increment_and_update_bounds(_x + 1, _y, _z, next_highest);
        out_expanded.set_cube_if_zero_and_increment_and_update_bounds(_x - 1, _y, _z, next_highest);
        out_expanded.set_cube_if_zero_and_increment_and_update_bounds(_x, _y + 1, _z, next_highest);
        out_expanded.set_cube_if_zero_and_increment_and_update_bounds(_x, _y - 1, _z, next_highest);
        out_expanded.set_cube_if_zero_and_increment_and_update_bounds(_x, _y, _z + 1, next_highest);
        out_expanded.set_cube_if_zero_and_increment_and_update_bounds(_x, _y, _z - 1, next_highest);
    }

    out_expanded.highest_written = next_highest - 1;

    if (!out_expanded.check_root())
    {
        printf("WTF?");
    }
}

/// <summary>
/// Crops a polycube to remove all outward planes with only zeros
/// </summary>
/// <param name="pc"></param>
/// <returns></returns>
inline void crop_cube(const rooted_polycube& base, rooted_polycube& out_cropped)
{
    if (!base.check_root())
    {
        printf("WTF?");
    }
    
    //Labelled max bounds stores max bounds of labelled polycubes (not filled, but have number)
    //So, we just shrink to those bounds
    int newX = base.labeled_max_bounds.x - base.labeled_min_bounds.x + 1;
    int newY = base.labeled_max_bounds.y - base.labeled_min_bounds.y + 1;
    int newZ = base.labeled_max_bounds.z - base.labeled_min_bounds.z + 1;

    out_cropped.k = base.k;
    out_cropped.highest_numbering = base.highest_numbering;
    out_cropped.highest_written = base.highest_written;
    out_cropped.dim = { (int8_t)newX, (int8_t)newY, (int8_t)newZ };
    
    //position delta = position{ (int8_t)(-minX), (int8_t)(-minY),(int8_t)(-minZ) };
    position delta = position{ (int8_t)(-base.labeled_min_bounds.x), (int8_t)(-base.labeled_min_bounds.y),(int8_t)(-base.labeled_min_bounds.z) };
    out_cropped.root = base.root + delta;
    out_cropped.min_bounds = base.min_bounds + delta;
    out_cropped.max_bounds = base.max_bounds + delta;
    out_cropped.labeled_min_bounds = base.labeled_min_bounds + delta;
    out_cropped.labeled_max_bounds = base.labeled_max_bounds + delta;
    
    out_cropped.filled_cubes = base.filled_cubes;
#ifdef _DEBUG
    out_cropped.debug_push_order = base.debug_push_order;
#endif

    memset(out_cropped.cubes, 0, out_cropped.size() * sizeof(out_cropped.cubes[0]));

    base.for_each_cube([&](int x, int y, int z, int16_t cube)
    {
        if (cube)
        {
            out_cropped.set_cube(x - base.labeled_min_bounds.x, y - base.labeled_min_bounds.y, z - base.labeled_min_bounds.z, cube);
        }
    });

    if (!out_cropped.check_root())
    {
        printf("WTF?");
    }
}

/// <summary>
/// Converts rooted_polycube to sparse polycube
/// </summary>
/// <param name="current"></param>
/// <returns></returns>
polycube_sparse get_polycube_sparse_from_rooted(const rooted_polycube& current)
{
    polycube_sparse pc;
    pc.num_cubes = current.filled_cubes.current;
    pc.dim = {  current.max_bounds.x - current.min_bounds.x + 1,
                current.max_bounds.y - current.min_bounds.y + 1,
                current.max_bounds.z - current.min_bounds.z + 1 };

    const position& min = current.min_bounds;
    position offset = { - min.x, - min.y, - min.z };
    current.for_each_filled([&](int x, int y, int z, size_t i)
    {
        pc.cubes[i] = position{ (int8_t)x, (int8_t)y, (int8_t)z } + offset;
    });
    return pc;
}

/// <summary>
/// Checks if rooted_polycube is canonical
/// </summary>
/// <param name="current"></param>
/// <param name="n"></param>
/// <param name="out_pc"></param>
/// <returns></returns>
bool is_canonical_sparse(const rooted_polycube& current, int n, polycube_sparse& out_pc)
{
    out_pc = get_polycube_sparse_from_rooted(current);
    return is_polycube_canonical_sparse(out_pc, n);
}


void print_rooted(const rooted_polycube* pc)
{
    std::stringstream ss;
    ss << (int)pc->dim.x << " " << (int)pc->dim.y << " " << (int)pc->dim.z << " : \n";
    ss << "Root =" << (int)pc->root.x << " " << (int)pc->root.y << " " << (int)pc->root.z << " : \n";

    for (int z = 0; z < pc->dim.z; z++)
    {
        for (int y = 0; y < pc->dim.y; y++)
        {
            for (int x = 0; x < pc->dim.x; x++)
            {
                if (pc->get_cube(x, y, z) == FILLED_CUBE)
                {
                    ss << "1 ";
                }
                else
                {
                    ss << "0 ";
                }
            }
            ss << "\n";
        }
        ss << "\n\n";
    }

    printf("%s\n", ss.str().c_str());
}


std::ostream& operator<< (std::ostream& out, const std::vector<int>& v)
{
    out << "{";
    for (int i = 0; i < v.size(); i++)
    {
        if (i > 0) out << ",";
        out << (int)v[i];
    }
    out << "}";
    return out << "\n";
}

template<typename OnFoundFunc, typename OnExpandedFunc>
size_t expand_polycubes_dfs_from_current(stack_allocator& allocator, int n, int m, rooted_polycube& current,  OnFoundFunc&& on_found, OnExpandedFunc&& on_expanded)
{
    stack_marker marker(allocator);
    rooted_polycube* expanded = allocator.allocate();

    expand_empty_slots(current, *expanded);
    rooted_polycube* cropped;

    //Since we only expand exactly what's needed, shouldn't have any extra to crop, once we reach a certain size
    if (current.k < 3)
    {
        cropped = allocator.allocate();
        crop_cube(*expanded, *cropped);
    } 
    else
    {
        cropped = expanded;
    }

    int highest_number = cropped->highest_numbering;
    size_t count = 0;
    position current_min = cropped->min_bounds;
    position current_max = cropped->max_bounds;

    if (!current.check_root())
    {
        printf("WTF???");
    }

    cropped->for_each_cube([&](int x, int y, int z, int cube)
    {
        if (cube != FILLED_CUBE && cube > cropped->highest_numbering)
        {
            //To go from rooted translation -> just translation, root must be on plane z = 0 and y = 0, and must be smallest x in that row
            if (z < cropped->root.z)
            {
                return;
            }
            else if (z == cropped->root.z && y < cropped->root.y)
            {
                return;
            }
            else if (z == cropped->root.z && y == cropped->root.y && x < cropped->root.x)
            {
                return;
            }

            cropped->k++;
            cropped->set_cube(x, y, z, FILLED_CUBE);
            cropped->highest_numbering = cube;

            position current = { (int8_t)x, (int8_t)y,(int8_t)z };
            position_min(cropped->min_bounds, current);
            position_max(cropped->max_bounds, current);

            cropped->filled_cubes.stack[cropped->filled_cubes.current] = { (int8_t)x - cropped->root.x, (int8_t)y - cropped->root.y, (int8_t)z - cropped->root.z };
            cropped->filled_cubes.current++;

#ifdef _DEBUG
            cropped->debug_push_order.push_back(cube);
#endif

            if (cropped->k == n)
            {
                position bounds = { cropped->max_bounds.x - cropped->min_bounds.x + 1,
                    cropped->max_bounds.y - cropped->min_bounds.y + 1,
                    cropped->max_bounds.z - cropped->min_bounds.z + 1 };

                //Canonical form assumes width >= height >= depth, so ignore cases where that isn't
                bool consider_this = true;
                if (bounds.z > bounds.x && bounds.z > bounds.y)
                {
                    //Depth is largest, ignore
                    consider_this = false;
                }
                else if (bounds.y > bounds.x && bounds.y > bounds.z)
                {
                    //Height is largest, ignore
                    consider_this = false;
                }
                else if (bounds.z > bounds.y)
                {
                    //Depth exceeds height
                    consider_this = false;
                } 

                if (consider_this)
                {

                    polycube_sparse pc;
                    bool canonical = is_canonical_sparse(*cropped, n, pc);

                    if (canonical)
                    {
                        count++;

                        on_found(pc);
                    }
                }
            }
            else if (cropped->k == m)
            {
                on_expanded(*cropped);
            }
            else
            {
                count += expand_polycubes_dfs_from_current(allocator, n, m, *cropped, on_found, on_expanded);
            }

#ifdef _DEBUG
            cropped->debug_push_order.pop_back();
#endif

            cropped->filled_cubes.current--;

            cropped->min_bounds = current_min;
            cropped->max_bounds = current_max;
            cropped->highest_numbering = highest_number;
            cropped->set_cube(x, y, z, cube);
            cropped->k--;
        }
    });

    return count;
}

/// <summary>
/// Expand polycubes using dfs
/// OnFoundFunc is const polycube_t& -> ()
/// OnExpandedFunc is const rooted_polycube& -> ()
/// m - size limit, if < n, calls on expanded instead of continuing search
/// </summary>
template<typename OnFoundFunc, typename OnExpandedFunc>
size_t expand_polycubes_dfs(stack_allocator& allocator, int n, int m, OnFoundFunc&& on_found, OnExpandedFunc&& on_expanded)
{
    if (n < 1)
    {
        return 0;
    }
    else if (n == 1 || n == 2)
    {
        return 1;
    }

    stack_marker marker(allocator);
    //proxy<rooted_polycube> next = allocator.allocate<rooted_polycube>();
    rooted_polycube* next = allocator.allocate();
    next->k = 1;
    next->root = { 0,0,0 };
    next->dim = { 1, 1, 1 };
    next->cubes[0] = FILLED_CUBE;
    next->highest_numbering = 1;
    next->highest_written = 1;
    next->min_bounds = { 0, 0, 0 };
    next->max_bounds = { 0, 0, 0 };
    next->labeled_min_bounds = { 0, 0, 0 };
    next->labeled_max_bounds = { 0, 0, 0 };

    next->filled_cubes.stack[0] = { 0,0,0 };
    next->filled_cubes.current = 1;

#ifdef _DEBUG
    next->debug_push_order.push_back(1);
#endif

    return expand_polycubes_dfs_from_current(allocator, n, m, *next,  on_found, on_expanded);
}


enum class job_type
{
    ExpandPolyCubes,
    EndProcess
};

struct queue_job
{
    job_type type = job_type::EndProcess;
    std::shared_ptr<void> data;
};

struct expand_poly_cubes_job
{
    rooted_polycube base;
    int n;
};

struct worker_thread_context
{
    thread_safe_queue<queue_job>* job_queue;
    thread_safe_queue<output_t>* output_queue;
    size_t stack_size;
};

/// <summary>
/// Worker thread function for polycube expander
/// </summary>
/// <param name="ctx"></param>
/// <param name="id"></param>
void polycubes_worker_thread(worker_thread_context ctx, int id)
{
    stack_allocator allocator;

    //printf("Starting Thread %d\n", id);
    bool running = true;
    while (running)
    {
        queue_job job;
        if (ctx.job_queue->dequeue(job))
        {
            switch (job.type)
            {
            case job_type::ExpandPolyCubes:
                scope {
                    expand_poly_cubes_job * expand_job = (expand_poly_cubes_job*)job.data.get();

                    printf("Expanding on thread %d\n", id);

                    size_t output = expand_polycubes_dfs_from_current(allocator, expand_job->n, expand_job->n, expand_job->base, [](auto&&) {}, [](auto&&) {});

                    ctx.output_queue->enqueue(output);
                }
                break;
            case job_type::EndProcess:
                running = false;
                break;
            }
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

/// <summary>
/// Thread pool for managin parallel execution of polycube expanders
/// </summary>
class polycubes_thread_pool
{

public:
    /// <summary>
    /// Initialized the threads in the pool
    /// </summary>
    /// <param name="k"></param>
    void init(size_t k)
    {
        if (k < 1)
        {
            printf("Error! no worker threads\n");
            return;
        }

        for (int i = 0; i < k; i++)
        {
            worker_thread_context context{ &m_job_queue, &m_output_queue, (size_t)-1 };
            m_worker_threads.push_back(std::thread(polycubes_worker_thread, context, i));
        }
    }

    /// <summary>
    /// Computes the number of polycubes of size n using parrallel threads
    /// </summary>
    /// <param name="base_cubes"></param>
    /// <param name="n"></param>
    /// <returns></returns>
    size_t generate_polycubes_parallel(int n)
    {
        stack_allocator allocator;

        int nodes_to_expand = 0;

        int EXPAND_SIZE_LIMIT = 5;
        if (n <= EXPAND_SIZE_LIMIT)
        {
            //Not big enough to care, expand single threaded
            return expand_polycubes_dfs(allocator, n, n, [](auto&&) {}, [](auto&&) {});
        }

        expand_polycubes_dfs(allocator, n, EXPAND_SIZE_LIMIT, [](auto&&) {}, [&](const rooted_polycube& pc) {
            nodes_to_expand++;

            std::shared_ptr<expand_poly_cubes_job> expand_job = std::make_shared<expand_poly_cubes_job>();
            expand_job->base = pc;
            expand_job->n = n;

            m_job_queue.enqueue(queue_job{ job_type::ExpandPolyCubes, expand_job });
        });
         
        //Do persistence to file and counting -> wait for m results from threads
        size_t num_polycubes = 0;

        for (int i = 0; i < nodes_to_expand; i++)
        {
            size_t result = m_output_queue.blocking_dequeue();

            num_polycubes += result;
        }

        return num_polycubes;
    }

    /// <summary>
    /// Shuts down all the threads in the pool
    /// </summary>
    void shutdown()
    {
        for (int i = 0; i < m_worker_threads.size(); i++)
        {
            m_job_queue.enqueue({ job_type::EndProcess, nullptr });
        }

        for (auto& thread : m_worker_threads)
        {
            thread.join();
        }
    }

private:
    //NOTE: could be high sources of contention
    thread_safe_queue<queue_job> m_job_queue;
    thread_safe_queue<output_t> m_output_queue;

    std::vector<std::thread> m_worker_threads;
};

/// <summary>
/// Returns all polycubes, threaded
/// </summary>
/// <param name="n"></param>
/// <param name="use_cache"></param>
/// <returns></returns>
inline size_t generate_polycubes_threaded(int n, polycubes_thread_pool& pool)
{
    if (n < 1)
    {
        return 0;
    }
    else if (n == 1)
    {
        return 1;
    }
    else if (n == 2)
    {
        return 1;
    }

    size_t num_cubes = 0;

    num_cubes = pool.generate_polycubes_parallel(n);

    printf("For n = {%d}, found {%llu} polycubes\n", n, num_cubes);

    return num_cubes;
}

