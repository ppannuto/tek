
/*
 * Copyright (C) 2011 Palmer Dabbelt
 *   <palmer@dabbelt.com>
 *
 * This file is part of tek.
 * 
 * tek is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * tek is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with tek.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PROCESSOR_PDFCROP_H
#define PROCESSOR_PDFCROP_H

#include "processor.h"

struct processor_pdfcrop
{
    struct processor p;
};

extern void processor_pdfcrop_boot(void *context);

extern struct processor *processor_pdfcrop_search(void *context,
                                                  const char *filename);

#endif
