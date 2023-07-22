#pragma once

#include <algorithm>
#include <cstdint>

/// <summary>
/// Small position struct for storing locations
/// </summary>
struct position 
{ 
    int8_t x, y, z;  
    int8_t _pad;
};

/// <summary>
/// Allows adding of positions together with '+' operator - makes parts of the code easier to read & write
/// </summary>
/// <param name="a"></param>
/// <param name="b"></param>
/// <returns></returns>
inline position operator+ (const position& a, const position& b)
{
    return { a.x + b.x, a.y + b.y, a.z + b.z };
}

/// <summary>
/// takes the component-wise max of two positions, and stores the value in inout_base
/// </summary>
/// <param name="inout_base"></param>
/// <param name="compare"></param>
inline void position_max(position& inout_base, const position& compare)
{
    inout_base.x = std::max(inout_base.x, compare.x);
    inout_base.y = std::max(inout_base.y, compare.y);
    inout_base.z = std::max(inout_base.z, compare.z);
}

/// <summary>
/// takes the component-wise min of two positions, and stores the value in inout_base
/// </summary>
/// <param name="inout_base"></param>
/// <param name="compare"></param>
inline void position_min(position& inout_base, const position& compare)
{
    inout_base.x = std::min(inout_base.x, compare.x);
    inout_base.y = std::min(inout_base.y, compare.y);
    inout_base.z = std::min(inout_base.z, compare.z);
}

/// <summary>
/// Struct for sparse polycube (only stores filled spaces, empty spaces ignored)
/// </summary>
struct polycube_sparse
{
    size_t num_cubes;

    position dim; //Dimensions of the polycube

    //Note: assumes max n of 32
    position cubes[32];

    inline polycube_sparse& operator=(const polycube_sparse& pc)
    {
        num_cubes = pc.num_cubes;
        dim = pc.dim;
        for (size_t i = 0; i < num_cubes; i++)
        {
            cubes[i] = pc.cubes[i];
        }
        return *this;
    }

    /// <summary>
    /// Calls a function on each cube in this polycube
    /// Func is (const position&, size_t) -> ()
    /// </summary>
    /// <typeparam name="Func"></typeparam>
    /// <param name="func"></param>
    template<typename Func>
    inline void for_each_cube(Func&& func) const
    {
        for (size_t i = 0; i < num_cubes; i++)
        {
            func(cubes[i], i);
        }
    }
};

/// <summary>
/// Returns smallest power of 10 greater than x, always returns value >= 1
/// </summary>
/// <param name="x"></param>
/// <returns></returns>
inline int log_smallest_power_of_10(int x)
{
    int power = 10;
    int log = 1;
    while (power <= x)
    {
        power *= 10;
        log++;
    }
    return log;
}

/// <summary>
/// Returns smallest power of 16 greater than x, always returns value >= 1
/// </summary>
/// <param name="x"></param>
/// <returns></returns>
inline int log_smallest_power_of_16(int x)
{
    int power = 16;
    int log = 1;
    while (power <= x)
    {
        power <<= 4;
        log++;
    }
    return log;
}

/// <summary>
/// Finds the string encoding of a sparse polycube, based on description from here: http://kevingong.com/Polyominoes/ParallelPoly.html
/// 
/// </summary>
/// <param name="pc"></param>
/// <param name="buffer"></param>
/// <param name="buf_size"></param>
inline void str_encoding_hex_sparse(const polycube_sparse& pc, char* buffer, size_t buf_size)
{
    size_t idx = 0;

    //this works because in ascii, numbers 0 - 9 are less than A-F
    const char map[] = "0123456789ABCDEF";

    //based on pc buffer of size 32
    int loc[32];

    for (int i = 0; i < pc.num_cubes; i++)
    {
        const position& c = pc.cubes[i];
        int label = c.z * (pc.dim.y * (int)pc.dim.x) + c.y * (int)pc.dim.x + c.x;

        //We have to sort the polycubes by number
        //The sparse polycubes aren't created in the required label order
        //So we need to adjust that by sorting the labels, as they need to be in increasing order
        int j = i;
        while (j > 0 && label < loc[j - 1])
        {
            loc[j] = loc[j - 1];
            j--;
        }
        loc[j] = label;
    }

    //int count = 0;
    for (int i = 0; i < pc.num_cubes; i++)
    {   
        int digits = log_smallest_power_of_16(loc[i]);
        int tmp = loc[i];
        for (int k = digits - 1; k >= 0; k--)
        {
            buffer[idx + k] = map[tmp & 0xF];
            tmp >>= 4;
        }
        idx += digits;
    }

    buffer[idx] = '\0';
}


const int X_AXIS = 0;
const int Y_AXIS = 1;
const int Z_AXIS = 2;

/// <summary>
/// Rotates a sparse polycube 90 degress around the specified axis
/// axis: 0 x axis, 1 y axis, 2 z axis
/// </summary>
/// <param name="pc"></param>
/// <param name="n"></param>
/// <param name="axis"></param>
/// <returns></returns>
template<int Axis>
inline polycube_sparse rotate_90_once_sparse(const polycube_sparse& pc)
{
    //rotation around z axis
    if (Axis == Z_AXIS)
    {
        polycube_sparse temp;
        temp.num_cubes = pc.num_cubes;
        temp.dim = { pc.dim.y, pc.dim.x, pc.dim.z };

        pc.for_each_cube([&](const position& cube, size_t i)
        {
            temp.cubes[i] = position{ pc.dim.y - 1 - cube.y, cube.x, cube.z };
        });

        return temp;
    }
    //Rotation around y axis
    else if (Axis == Y_AXIS)
    {
        polycube_sparse temp;
        temp.num_cubes = pc.num_cubes;
        temp.dim = { pc.dim.z , pc.dim.y, pc.dim.x };


        pc.for_each_cube([&](const position& cube, size_t i)
        {
            temp.cubes[i] = position{cube.z, cube.y, pc.dim.x - 1 - cube.x };
        });

        return temp;

    }
    //Rotation around x axis
    else if (Axis == X_AXIS)
    {
        polycube_sparse temp;
        temp.num_cubes = pc.num_cubes;
        temp.dim = { pc.dim.x, pc.dim.z , pc.dim.y };

        pc.for_each_cube([&](const position& cube, size_t i)
        {
            temp.cubes[i] = position{ cube.x, pc.dim.z - 1 - cube.z, cube.y };
        });

        return temp;
    }

    printf("ERROR: Invalid axes\n");
    return polycube_sparse{ 0 };
}


/// <summary>
/// Rotates a sparse polycube -90 degress around the specified axis
/// axis: 0 x axis, 1 y axis, 2 z axis
/// </summary>
/// <param name="pc"></param>
/// <param name="n"></param>
/// <param name="axis"></param>
/// <returns></returns>
template<int Axis>
inline polycube_sparse rotate_90_reverse_sparse(const polycube_sparse& pc)
{
    //rotation around z axis
    if (Axis == Z_AXIS)
    {
        polycube_sparse temp;
        temp.num_cubes = pc.num_cubes;
        temp.dim = { pc.dim.y, pc.dim.x, pc.dim.z };

        pc.for_each_cube([&](const position& cube, size_t i)
        {
            temp.cubes[i] = position{ cube.y,  pc.dim.x - 1 - cube.x, cube.z };
        });

        return temp;
    }
    //Rotation around y axis
    else if (Axis == Y_AXIS)
    {
        polycube_sparse temp;
        temp.num_cubes = pc.num_cubes;
        temp.dim = { pc.dim.z , pc.dim.y, pc.dim.x };

        pc.for_each_cube([&](const position& cube, size_t i)
        {
            temp.cubes[i] = position{ pc.dim.z - 1 - cube.z, cube.y,  cube.x };
        });

        return temp;

    }
    //Rotation around x axis
    else if (Axis == X_AXIS)
    {
        polycube_sparse temp;
        temp.num_cubes = pc.num_cubes;
        temp.dim = { pc.dim.x, pc.dim.z , pc.dim.y};


        pc.for_each_cube([&](const position& cube, size_t i)
        {
            temp.cubes[i] = position{ cube.x, cube.z, pc.dim.y - 1 - cube.y };
        });

        return temp;
    }

    printf("ERROR: Invalid axes\n");
    return polycube_sparse{ 0 };
}

/// <summary>
/// Rotates a sparse polycube 180 degress around the specified axis
/// axis: 0 x axis, 1 y axis, 2 z axis
/// </summary>
/// <param name="pc"></param>
/// <param name="n"></param>
/// <param name="axis"></param>
/// <returns></returns>
template<int Axis>
inline polycube_sparse rotate_twice_sparse(const polycube_sparse& pc)
{
    //rotation around z axis
    if (Axis == Z_AXIS)
    {
        polycube_sparse temp;
        temp.num_cubes = pc.num_cubes;
        temp.dim = pc.dim;

        pc.for_each_cube([&](const position& cube, size_t i)
        {
            temp.cubes[i] = position{ pc.dim.x - 1 - cube.x, pc.dim.y - 1 - cube.y, cube.z };
        });

        return temp;
    }
    //Rotation around y axis
    else if (Axis == Y_AXIS)
    {
        polycube_sparse temp;
        temp.num_cubes = pc.num_cubes;
        temp.dim = pc.dim;

        pc.for_each_cube([&](const position& cube, size_t i)
        {
            temp.cubes[i] = position{ pc.dim.x - 1 - cube.x, cube.y,  pc.dim.z - 1 - cube.z };
        });

        return temp;

    }
    //Rotation around x axis
    else if (Axis == X_AXIS)
    {
        polycube_sparse temp;
        temp.num_cubes = pc.num_cubes;
        temp.dim = pc.dim;

        pc.for_each_cube([&](const position& cube, size_t i)        {

            temp.cubes[i] = position{ cube.x, pc.dim.y - 1 - cube.y,  pc.dim.z - 1 - cube.z };
        });

        return temp;
    }

    printf("ERROR: Invalid axes\n");
    return polycube_sparse{ 0 };
}

/// <summary>
/// An class that iterates through all the rotations of a sparse polycube
/// </summary>
class all_rotations_generator_sparse
{

public:

    all_rotations_generator_sparse(const polycube_sparse& cube) :
        m_original(cube), m_index(0)
    {
        m_original = cube;
    }

    inline bool has_next() const
    {
        return m_index < 24;
    }

    inline polycube_sparse& next()
    {

        if (m_index < 4)
        {
            if (m_index == 0)
            {
                m_base = m_original;
            }
            else
            {
                m_base = rotate_90_once_sparse<X_AXIS>(m_base);
            }
        }
        else if (m_index < 8)
        {
            if (m_index == 4)
            {
                m_base = rotate_twice_sparse<Y_AXIS>(m_base);
            }
            else
            {
                m_base = rotate_90_once_sparse<X_AXIS>(m_base);
            }
        }
        else if (m_index < 12)
        {
            if (m_index == 8)
            {
                m_base = rotate_90_once_sparse<Y_AXIS>(m_original);;
            }
            else
            {
                m_base = rotate_90_once_sparse<Z_AXIS>(m_base);
            }
        }
        else if (m_index < 16)
        {
            if (m_index == 12)
            {
                m_base = rotate_90_reverse_sparse<Y_AXIS>(m_original);
            }
            else
            {
                m_base = rotate_90_once_sparse<Z_AXIS>(m_base);
            }
        }
        else if (m_index < 20)
        {
            if (m_index == 16)
            {
                m_base = rotate_90_once_sparse<Z_AXIS>(m_original);
            }
            else
            {
                m_base = rotate_90_once_sparse<Y_AXIS>(m_base);
            }
        }
        else if (m_index < 24)
        {
            if (m_index == 20)
            {
                m_base = rotate_90_reverse_sparse<Z_AXIS>(m_original);
            }
            else
            {
                m_base = rotate_90_once_sparse<Y_AXIS>(m_base);
            }
        }
        m_index++;
        return m_base;
    }

    inline int get_current_index() const
    {
        return m_index;
    }

    inline void set_current_index(int index)
    {
        if (index % 4 != 0)
        {
            m_index = index & (~0x03);
            for (int i = 0; i < index % 4; i++)
            {
                next();
            }
        }
        else
        {
            m_index = index;
        }
    }

private:

    polycube_sparse m_original;
    polycube_sparse m_base;

    int m_index;
};

/// <summary>
/// A class that iterates through the rotations of a sparse polycube, only obtained from 180 degree rotations about axes
/// </summary>
class all_180_rotations_generator_sparse
{

public:

    all_180_rotations_generator_sparse(const polycube_sparse& cube) :
        m_original(cube), m_index(0)
    {
        m_original = cube;
    }

    bool has_next()
    {
        return m_index < 8;
    }

    inline polycube_sparse& next()
    {

        if (m_index == 0)
        {
            m_base = m_original;
        }
        else //each reflection is 1, or 0, so binary scheme
            //to generate efficiently, use gray code scheme encoding of rotations (subsequent numbers differ by exactly one bit)
            //m_base starts at 000 (z-axis, y-axis, x-axis)
            //Whenever the bit changes, reflect along that axis
        {
            switch (m_index)
            {
            case 1: //001
                m_base = rotate_twice_sparse<X_AXIS>(m_base);
                break;
            case 2: //011
                m_base = rotate_twice_sparse<Y_AXIS>(m_base);
                break;
            case 3: //010
                m_base = rotate_twice_sparse<X_AXIS>(m_base);
                break;
            case 4: //110
                m_base = rotate_twice_sparse<Z_AXIS>(m_base);
                break;
            case 5: //111
                m_base = rotate_twice_sparse<X_AXIS>(m_base);
                break;
            case 6: //101
                m_base = rotate_twice_sparse<Y_AXIS>(m_base);
                break;
            case 7: //100
                m_base = rotate_twice_sparse<X_AXIS>(m_base);
                break;
            }
        }

        m_index++;
        return m_base;
    }

    inline int get_current_index() const
    {
        return m_index;
    }

private:

    polycube_sparse m_original;
    polycube_sparse m_base;

    int m_index;
};

/// <summary>
/// Checks if sparse polycube is canonical
/// </summary>
/// <param name="pc"></param>
/// <param name="n"></param>
/// <returns></returns>
inline bool is_polycube_canonical_sparse(const polycube_sparse& pc, int n)
{

    if (pc.dim.x < pc.dim.y || pc.dim.x < pc.dim.z || pc.dim.y < pc.dim.z)
    {
        return false;
    }

    //std::string pc_encoding = str_encoding(pc, n);
    char pc_encoding[1024];
    str_encoding_hex_sparse(pc, pc_encoding, sizeof(pc_encoding) / sizeof(char));

    //All dim are unique, only need to check eight orientations
    if (pc.dim.x != pc.dim.y && pc.dim.x != pc.dim.z && pc.dim.y != pc.dim.z)
    {
        polycube_sparse temp = pc;
        //z largest, switch z and y
        if (pc.dim.z >= pc.dim.x && pc.dim.z >= pc.dim.y)
        {
            temp = rotate_90_once_sparse<Y_AXIS>(temp);
        }
        //y largest, switch x and y
        else if (pc.dim.y >= pc.dim.x && pc.dim.y >= pc.dim.z)
        {
            temp = rotate_90_once_sparse<Z_AXIS>(temp);
        }
        //Check y greater than z
        if (pc.dim.y < pc.dim.z)
        {
            temp = rotate_90_once_sparse<X_AXIS>(temp);
        }

        all_180_rotations_generator_sparse gen(temp);

        //Now, doing comparisons
        while (gen.has_next())
        {
            polycube_sparse& cube = gen.next();

            char cube_repr[1024];
            str_encoding_hex_sparse(cube, cube_repr, sizeof(cube_repr) / sizeof(char));
            if (strcmp(cube_repr, pc_encoding) < 0)
            {
                return false;
            }
        }

        return true;
    }

    all_rotations_generator_sparse gen(pc);

    //Now, doing comparisons
    while (gen.has_next())
    {
        polycube_sparse& cube = gen.next();

        if (cube.dim.x >= cube.dim.y && cube.dim.y >= cube.dim.z)
        {

            char cube_repr[1024];
            str_encoding_hex_sparse(cube, cube_repr, sizeof(cube_repr) / sizeof(char));

            if (strcmp(cube_repr, pc_encoding) < 0)
            {
                return false;
            }
        }
    }

    return true;
}