#ifndef I_POP_UP_H
#define I_POP_UP_H

namespace openflash
{
    namespace gba
    {
        class i_pop_up
        {
        public:
            virtual ~i_pop_up() = default;
            virtual void render() = 0;
            virtual void update() = 0;
            virtual void dismiss() = 0;
        };
    }
}

#endif // I_POP_UP_H