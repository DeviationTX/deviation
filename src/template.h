#ifndef _TEMPLATE_H_
#define _TEMPLATE_H_

enum Templates {
    TEMPLATE_NONE,
    TEMPLATE_4CH_SIMPLE,
    TEMPLATE_6CH_PLANE,
    TEMPLATE_6CH_HELI,
};
#define NUM_TEMPLATES 4

void TEMPLATE_Apply(enum Templates tmpl);
#endif
