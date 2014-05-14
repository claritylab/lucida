/*
 TinySVM -- Yet Another Tiny SVM Package

 $Id: svr_solver.h,v 1.5 2002/08/20 06:31:17 taku-ku Exp $;

 Copyright (C) 2001-2002  Taku Kudo <taku-ku@is.aist-nara.ac.jp>
 All rights reserved.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License as published by the Free Software Foundation; either
 version 2 of the License, or (at your option) any later verjsion.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.

 You should have received a copy of the GNU Library General Public
 License along with this library; if not, write to the
 Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 Boston, MA 02111-1307, USA.
*/


#ifndef _SVR_SOLVER_H
#define _SVR_SOLVER_H
#include "cache.h"
#include "param.h"
#include "base_example.h"
#include "base_solver.h"

// $Id: svr_solver.h,v 1.5 2002/08/20 06:31:17 taku-ku Exp $;

namespace TinySVM {

class SVR_Solver: public BaseSolver
{
public:
  SVR_Solver(const BaseExample &e, const Param &p): BaseSolver(e,p) {};
  ~SVR_Solver() {};

  Model *learn();
};


};
#endif

