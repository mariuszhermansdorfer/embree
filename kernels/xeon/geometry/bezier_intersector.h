// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#pragma once

#include "../../common/ray.h"
#include "../../common/globals.h"
#include "filter.h"
#include "cylinder.h"
#include "cone.h"
#include "plane.h"
#include "line_intersector.h"

extern "C" int g_debug_int0;
extern "C" int g_debug_int1;

namespace embree
{
  namespace isa
  {
    __forceinline bool intersect_bezier_iterative_jacobian(const Ray& ray, const BezierCurve3fa& curve, float u, float t, float& u_o, float& t_o, Vec3fa& Ng_o)
    {
      const float length_ray_dir = length(ray.dir);
      for (size_t i=0; i<g_debug_int1; i++) 
      {
        const Vec3fa Q = ray.org + t*ray.dir;
        //const Vec3fa dQdu = zero;
        const Vec3fa dQdt = ray.dir;

        Vec3fa P,dPdu,ddPdu; curve.eval(u,P,dPdu,ddPdu);
        //const Vec3fa dPdt = zero;

        const Vec3fa R = Q-P;
        const Vec3fa dRdu = /*dQdu*/-dPdu;
        const Vec3fa dRdt = dQdt;//-dPdt;

        const Vec3fa T = normalize(dPdu);
        const Vec3fa dTdu = dnormalize(dPdu,ddPdu);
        //const Vec3fa dTdt = zero;

        const float f = dot(R,T);
        const float dfdu = dot(dRdu,T) + dot(R,dTdu);
        const float dfdt = dot(dRdt,T);// + dot(R,dTdt);

        const float K = dot(R,R)-sqr(f);
        const float dKdu = /*2.0f*/(dot(R,dRdu)-f*dfdu);
        const float dKdt = /*2.0f*/(dot(R,dRdt)-f*dfdt);

        const float g = sqrt(K)-P.w;
        const float dgdu = /*0.5f*/dKdu*rsqrt(K)-dPdu.w;
        const float dgdt = /*0.5f*/dKdt*rsqrt(K);//-dPdt.w;

        const LinearSpace2f J = LinearSpace2f(dfdu,dfdt,dgdu,dgdt);
        const Vec2f dut = rcp(J)*Vec2f(f,g);
        const Vec2f ut = Vec2f(u,t) - dut;
        u = ut.x; t = ut.y;
        
        if (abs(f) < 16.0f*float(ulp)*reduce_max(abs(dPdu)) && abs(g) < 16.0f*float(ulp)*length_ray_dir) 
        {
          if (t > t_o) return false;
          if (t < ray.tnear || t > ray.tfar) return false;
          if (u < 0.0f || u_o > 1.0f) return false;
          u_o = u;
          t_o = t;
          const Vec3fa R = normalize(Q-P);
          const Vec3fa U = dPdu+dPdu.w*R;
          const Vec3fa V = cross(dPdu,R);
          Ng_o = cross(V,U);
          return true;
        }
      }
      return false;
    }

    __forceinline bool intersect_bezier_iterative_debug(const Ray& ray, const BezierCurve3fa& curve, size_t i, 
                                                        const vfloatx& u, const BBox<vfloatx>& tp, const BBox<vfloatx>& h0, const BBox<vfloatx>& h1, 
                                                        const Vec3vfx& Ng, const Vec4vfx& dP0du, const Vec4vfx& dP3du,
                                                        float& u_o, float& t_o, Vec3fa& Ng_o)
    {
      if (tp.lower[i] >= t_o) return false;
      u_o = u[i];
      t_o = tp.lower[i];
      Ng_o = Vec3fa(Ng.x[i],Ng.y[i],Ng.z[i]);
      if (h0.lower[i] == tp.lower[i]) Ng_o = -Vec3fa(dP0du.x[i],dP0du.y[i],dP0du.z[i]);
      if (h1.lower[i] == tp.lower[i]) Ng_o = +Vec3fa(dP3du.x[i],dP3du.y[i],dP3du.z[i]);
      return true;
    }

    __forceinline bool intersect_bezier_iterative_step(const Ray& ray, const BezierCurve3fa& curve, float u0, float u1, float t0, float t1, float t2, float& u_o, float& t_o, Vec3fa& Ng_o)
    {
      Vec3fa p0,n0,ddp0; curve.eval(u0,p0,n0,ddp0);
      Vec3fa p1,n1,ddp1; curve.eval(u1,p1,n1,ddp1);

      Vec3fa q0 = p0+n0*(1.0f/3.0f);
      Vec3fa q1 = p1-n1*(1.0f/3.0f);
      float rq0 = length(cross(p0-q0,p1-q0))/length(p1-p0)+q0.w;
      float rq1 = length(cross(p0-q1,p1-q1))/length(p1-p0)+q1.w;
      float r01 = max(p0.w,rq0,rq1,p1.w);

      /* calculate bounds on curvature */
      float minN = min(length(n0),length(n1));
      float maxN = max(length(n0),length(n1));
      float maxR = max(p0.w,q0.w,q1.w,p1.w);
      float maxC = length(max(abs(ddp0),abs(ddp1)))/minN; //length(ddPdu)/length(dPdu); // FIXME: is this the proper curvature?
      float minDT = max(0.001f,1.0f/maxC-maxR);

      /* iterative double step solver */
      float eps = 128.0f*float(ulp);
      float rcpLenP0P1 = rcp_length(p1-p0);
      float rcpLenDir = rcp_length(ray.dir);
      float t = t0;
      float u = u0 + (u1-u0)*dot(ray.org+t*ray.dir-p0,normalize(p1-p0))*rcpLenP0P1;
      for (size_t i=0; i<10000; i++)
      {
        Vec3fa Q = ray.org + t*ray.dir;
        Vec3fa P,dPdu,ddPdu; curve.eval(u,P,dPdu,ddPdu);
        Vec3fa T = normalize(dPdu); // can be optimized away
        //float C = length(ddPdu)/length(dPdu); // FIXME: is this the proper curvature?
        float du = dot(Q-P,T);
        float dt = sqrt(dot(Q-P,Q-P)-sqr(du))-P.w;
        u += du*rcpLenP0P1*(u1-u0)/(1.0f+P.w*maxC);
        //if (du < 10.0f*eps)
        //t += min(minDT,dt*rcpLenDir);
        t += dt*rcpLenDir;

        if (t > t2) {
          return false;
        }
        if (max(abs(du),abs(dt)) < eps) 
        {
          //if (t+eps*rcpLenDir < t0 || t-eps*rcpLenDir > t1) {
          //  return false;
          //}
          if (t < ray.tnear || t > t_o) {
            return false;
          }
          u_o = u;
          t_o = t;
          Ng_o = Q-P;
          return true;
        }
      }
      return false;
    }

    __forceinline bool intersect_bezier_recursive_jacobian(const Ray& ray, const BezierCurve3fa& curve, const float u0, const float u1, const size_t depth, float& u_o, float& t_o, Vec3fa& Ng_o)
    {
      const int maxDepth = g_debug_int0;

      /* subdivide curve */
      const float dscale = (u1-u0)*(1.0f/(3.0f*(VSIZEX-1)));
      const vfloatx vu0 = lerp(u0,u1,vfloatx(step)*(1.0f/(VSIZEX-1)));
      Vec4vfx P0, dP0du; curve.evalN(vu0,P0,dP0du); dP0du = dP0du * Vec4vfx(dscale);
      const Vec4vfx P3 = shift_right_1(P0);
      const Vec4vfx dP3du = Vec4vfx(dscale)*shift_right_1(dP0du); 
      const Vec4vfx P1 = P0 + dP0du; 
      const Vec4vfx P2 = P3 - dP3du;

      /* calculate bounding cylinders */
      const vfloatx rr1 = sqr_point_to_line_distance(Vec3vfx(dP0du),Vec3vfx(P3-P0));
      const vfloatx rr2 = sqr_point_to_line_distance(Vec3vfx(dP3du),Vec3vfx(P3-P0));
      const vfloatx maxr12 = sqrt(max(rr1,rr2));
      const vfloatx r_outer = max(P0.w,P1.w,P2.w,P3.w)+maxr12;
      const vfloatx r_inner = max(0.0f,min(P0.w,P1.w,P2.w,P3.w)-maxr12);
      const CylinderN<VSIZEX> cylinder_outer(Vec3vfx(P0),Vec3vfx(P3),r_outer);
      const CylinderN<VSIZEX> cylinder_inner(Vec3vfx(P0),Vec3vfx(P3),r_inner);
      vboolx valid = true; clear(valid,VSIZEX-1);

      /* intersect with outer cylinder */
      BBox<vfloatx> tc_outer; vfloatx u_outer0; Vec3vfx Ng_outer0; vfloatx u_outer1; Vec3vfx Ng_outer1;
      valid &= cylinder_outer.intersect(ray.org,ray.dir,tc_outer,u_outer0,Ng_outer0,u_outer1,Ng_outer1);
      if (none(valid)) return false;

      /* intersect with cap-planes */
      BBox<vfloatx> tp(ray.tnear,min(t_o,ray.tfar));
      tp = embree::intersect(tp,tc_outer);
      BBox<vfloatx> h0 = HalfPlaneN<VSIZEX>(Vec3vfx(P0),+Vec3vfx(dP0du)).intersect(ray.org,ray.dir);
      tp = embree::intersect(tp,h0);
      BBox<vfloatx> h1 = HalfPlaneN<VSIZEX>(Vec3vfx(P3),-Vec3vfx(dP3du)).intersect(ray.org,ray.dir);
      tp = embree::intersect(tp,h1);
      valid &= tp.lower <= tp.upper;
      if (none(valid)) return false;

      /* clamp and correct u parameter */
      u_outer0 = clamp(u_outer0,vfloatx(0.0f),vfloatx(1.0f));
      u_outer1 = clamp(u_outer1,vfloatx(0.0f),vfloatx(1.0f));
      u_outer0 = lerp(u0,u1,(vfloatx(step)+u_outer0)*(1.0f/float(VSIZEX)));
      u_outer1 = lerp(u0,u1,(vfloatx(step)+u_outer1)*(1.0f/float(VSIZEX)));

      /* intersect with inner cylinder */
      BBox<vfloatx> tc_inner;
      cylinder_inner.intersect(ray.org,ray.dir,tc_inner);

      /* subtract the inner interval from the current hit interval */
      BBox<vfloatx> tp0, tp1;
      subtract(tp,tc_inner,tp0,tp1);
      vboolx valid0 = valid & (tp0.lower <= tp0.upper);
      vboolx valid1 = valid & (tp1.lower <= tp1.upper);
      if (none(valid0 | valid1)) return false;

      /* iterate over all first hits front to back */
      bool found = false;
      while (any(valid0))
      {
        const size_t i = select_min(valid0,tp0.lower); clear(valid0,i);
        //if (depth == maxDepth) found |= intersect_bezier_iterative_jacobian(ray,curve,u_outer0[i],tp0.lower[i],u_o,t_o,Ng_o);
        if (depth == maxDepth) found |= intersect_bezier_iterative_debug   (ray,curve,i,u_outer0,tp0,h0,h1,Ng_outer0,dP0du,dP3du,u_o,t_o,Ng_o);
        else                   found |= intersect_bezier_recursive_jacobian(ray,curve,vu0[i+0],vu0[i+1],depth+1,u_o,t_o,Ng_o);
        valid0 &= tp0.lower < t_o;
      }

      /* iterate over all second hits front to back */
      while (any(valid1))
      {
        const size_t i = select_min(valid1,tp1.lower); clear(valid1,i);
        //if (depth == maxDepth) found |= intersect_bezier_iterative_jacobian(ray,curve,u_outer1[i],tp1.upper[i],u_o,t_o,Ng_o);
        if (depth == maxDepth) found |= intersect_bezier_iterative_debug   (ray,curve,i,u_outer1,tp1,h0,h1,Ng_outer1,dP0du,dP3du,u_o,t_o,Ng_o);
        else                   found |= intersect_bezier_recursive_jacobian(ray,curve,vu0[i+0],vu0[i+1],depth+1,u_o,t_o,Ng_o);
        valid1 &= tp1.lower < t_o;
      }
      return found;
    }

    __forceinline bool intersect_bezier_recursive_cone(const Ray& ray, const BezierCurve3fa& curve, const float u0, const float u1, const size_t depth, float& u_o, float& t_o, Vec3fa& Ng_o)
    {
      const int maxDepth = g_debug_int0;

      /* subdivide curve */
      const float dscale = (u1-u0)*(1.0f/(3.0f*(VSIZEX-1)));
      const vfloatx vu0 = lerp(u0,u1,vfloatx(step)*(1.0f/(VSIZEX-1)));
      Vec4vfx P0, dP0du; curve.evalN(vu0,P0,dP0du); dP0du = dP0du * Vec4vfx(dscale);
      const Vec4vfx P3 = shift_right_1(P0);
      const Vec4vfx dP3du = Vec4vfx(dscale)*shift_right_1(dP0du); 
      const Vec4vfx P1 = P0 + dP0du; 
      const Vec4vfx P2 = P3 - dP3du;

      /* calculate bounding cylinders */
      const vfloatx rr1 = sqr_point_to_line_distance(Vec3vfx(dP0du),Vec3vfx(P3-P0));
      const vfloatx rr2 = sqr_point_to_line_distance(Vec3vfx(dP3du),Vec3vfx(P3-P0));
      const vfloatx maxr12 = sqrt(max(rr1,rr2));
      const vfloatx r_outer = max(P0.w,P1.w,P2.w,P3.w)+maxr12;
      const vfloatx r_inner = max(0.0f,min(P0.w,P1.w,P2.w,P3.w)-maxr12);
      const CylinderN<VSIZEX> cylinder_outer(Vec3vfx(P0),Vec3vfx(P3),r_outer);
      const CylinderN<VSIZEX> cylinder_inner(Vec3vfx(P0),Vec3vfx(P3),r_inner);
      vboolx valid = true; clear(valid,VSIZEX-1);

      /* put cones at the leaf level */
      if (depth == maxDepth) 
      {
        const ConeN<VSIZEX> cone(Vec3vfx(P0),P0.w,Vec3vfx(P3),P3.w);
        BBox<vfloatx> tc; vfloatx uc0; Vec3vfx Ng0; //vfloatx uc1; Vec3vfx Ng1;
        valid &= cone.intersect(ray.org,ray.dir,tc,uc0,Ng0); //,uc1,Ng1);
        if (none(valid)) return false;

        /* intersect with cap-planes */
        BBox<vfloatx> tp(ray.tnear,min(t_o,ray.tfar));
        tp = embree::intersect(tp,tc);
        BBox<vfloatx> h0 = HalfPlaneN<VSIZEX>(Vec3vfx(P0),+Vec3vfx(dP0du)).intersect(ray.org,ray.dir);
        tp = embree::intersect(tp,h0);
        BBox<vfloatx> h1 = HalfPlaneN<VSIZEX>(Vec3vfx(P3),-Vec3vfx(dP3du)).intersect(ray.org,ray.dir);
        tp = embree::intersect(tp,h1);
        valid &= tp.lower <= tp.upper;
        if (none(valid)) return false;
        
        /* clamp and correct u parameter */
        uc0 = clamp(uc0,vfloatx(0.0f),vfloatx(1.0f));
        //uc1 = clamp(uc1,vfloatx(0.0f),vfloatx(1.0f));
        uc0 = lerp(u0,u1,(vfloatx(step)+uc0)*(1.0f/float(VSIZEX)));
        //uc1 = lerp(u0,u1,(vfloatx(step)+uc1)*(1.0f/float(VSIZEX)));

        valid &= tp.lower < t_o;
        if (none(valid)) return false;

        const size_t i = select_min(valid,tp.lower); 
        u_o = uc0[i];
        t_o = tp.lower[i];
        Ng_o = Vec3fa(Ng0.x[i],Ng0.y[i],Ng0.z[i]);
        if (h0.lower[i] == tp.lower[i]) Ng_o = -Vec3fa(dP0du.x[i],dP0du.y[i],dP0du.z[i]);
        if (h1.lower[i] == tp.lower[i]) Ng_o = +Vec3fa(dP3du.x[i],dP3du.y[i],dP3du.z[i]);
        return true;
      }

      /* intersect with outer cylinder */
      BBox<vfloatx> tc_outer; vfloatx u_outer0; Vec3vfx Ng_outer0; vfloatx u_outer1; Vec3vfx Ng_outer1;
      valid &= cylinder_outer.intersect(ray.org,ray.dir,tc_outer,u_outer0,Ng_outer0,u_outer1,Ng_outer1);
      if (none(valid)) return false;

      /* intersect with cap-planes */
      BBox<vfloatx> tp(ray.tnear,min(t_o,ray.tfar));
      tp = embree::intersect(tp,tc_outer);
      BBox<vfloatx> h0 = HalfPlaneN<VSIZEX>(Vec3vfx(P0),+Vec3vfx(dP0du)).intersect(ray.org,ray.dir);
      tp = embree::intersect(tp,h0);
      BBox<vfloatx> h1 = HalfPlaneN<VSIZEX>(Vec3vfx(P3),-Vec3vfx(dP3du)).intersect(ray.org,ray.dir);
      tp = embree::intersect(tp,h1);
      valid &= tp.lower <= tp.upper;
      if (none(valid)) return false;

      /* clamp and correct u parameter */
      u_outer0 = clamp(u_outer0,vfloatx(0.0f),vfloatx(1.0f));
      u_outer1 = clamp(u_outer1,vfloatx(0.0f),vfloatx(1.0f));
      u_outer0 = lerp(u0,u1,(vfloatx(step)+u_outer0)*(1.0f/float(VSIZEX)));
      u_outer1 = lerp(u0,u1,(vfloatx(step)+u_outer1)*(1.0f/float(VSIZEX)));

      /* intersect with inner cylinder */
      BBox<vfloatx> tc_inner;
      cylinder_inner.intersect(ray.org,ray.dir,tc_inner);

      /* subtract the inner interval from the current hit interval */
      BBox<vfloatx> tp0, tp1;
      subtract(tp,tc_inner,tp0,tp1);
      vboolx valid0 = valid & (tp0.lower <= tp0.upper);
      vboolx valid1 = valid & (tp1.lower <= tp1.upper);
      if (none(valid0 | valid1)) return false;

      /* iterate over all first hits front to back */
      bool found = false;
      while (any(valid0))
      {
        const size_t i = select_min(valid0,tp0.lower); clear(valid0,i);
        found |= intersect_bezier_recursive_cone(ray,curve,vu0[i+0],vu0[i+1],depth+1,u_o,t_o,Ng_o);
        valid0 &= tp0.lower < t_o;
      }

      /* iterate over all second hits front to back */
      while (any(valid1))
      {
        const size_t i = select_min(valid1,tp1.lower); clear(valid1,i);
        found |= intersect_bezier_recursive_cone(ray,curve,vu0[i+0],vu0[i+1],depth+1,u_o,t_o,Ng_o);
        valid1 &= tp1.lower < t_o;
      }
      return found;
    }

    template<int M>
      struct BezierHit
    {
      __forceinline BezierHit() {}

      __forceinline BezierHit(const vbool<M>& valid, const vfloat<M>& U, const vfloat<M>& V, const vfloat<M>& T, const int i, const int N,
                              const Vec3fa& p0, const Vec3fa& p1, const Vec3fa& p2, const Vec3fa& p3)
        : valid(valid), U(U), V(V), T(T), i(i), N(N), p0(p0), p1(p1), p2(p2), p3(p3) {}
      
      __forceinline void finalize() 
      {
        vu = (vfloat<M>(step)+U+vfloat<M>(i))*(1.0f/float(N));
        vv = 0.0f;
        vt = T;
      }
      
      __forceinline Vec2f uv (const size_t i) const { return Vec2f(vu[i],vv[i]); }
      __forceinline float t  (const size_t i) const { return vt[i]; }
      __forceinline Vec3fa Ng(const size_t i) const 
      { 
        Vec3fa T = BezierCurve3fa(p0,p1,p2,p3,0.0f,1.0f,0).eval_du(vu[i]);
        return T == Vec3fa(zero) ? Vec3fa(one) : T; 
      }
      
    public:
      vfloat<M> U;
      vfloat<M> V;
      vfloat<M> T;
      int i, N;
      Vec3fa p0,p1,p2,p3;
      
    public:
      vbool<M> valid;
      vfloat<M> vu;
      vfloat<M> vv;
      vfloat<M> vt;
    };
    
#if 1

    struct Bezier1Intersector1
    {
      __forceinline Bezier1Intersector1(const Ray& ray, const void* ptr) {}

      template<typename Epilog>
      __forceinline bool intersect(Ray& ray,
                                   const Vec3fa& v0, const Vec3fa& v1, const Vec3fa& v2, const Vec3fa& v3, const int Np,
                                   const Epilog& epilog) const
      {
        STAT3(normal.trav_prims,1,1,1);

        /* move ray closer to make intersection stable */
        const float dt = dot(0.25f*(v0+v1+v2+v3)-ray.org,ray.dir)*rcp(dot(ray.dir,ray.dir));
        const Vec3fa ref(ray.org+dt*ray.dir,0.0f);
        const Vec3fa p0 = v0-ref;
        const Vec3fa p1 = v1-ref;
        const Vec3fa p2 = v2-ref;
        const Vec3fa p3 = v3-ref;

        float u = 0.0f;
        float t = ray.tfar-dt;
        Vec3fa Ng = zero;

        const Ray ray1(zero,ray.dir,ray.tnear-dt,ray.tfar-dt);
        const BezierCurve3fa curve(p0,p1,p2,p3,0.0f,1.0f,1);
        if (!intersect_bezier_recursive_jacobian(ray1,curve,0.0f,1.0f,1,u,t,Ng))
        //if (!intersect_bezier_recursive_cone(ray1,curve,0.0f,1.0f,1,u,t,Ng))
          return false;

        LineIntersectorHitM<VSIZEX> hit;
        hit.vu[0] = u;
        hit.vv[0] = 0.0f;
        hit.vt[0] = t+dt;
        hit.vNg.x[0] = Ng.x;
        hit.vNg.y[0] = Ng.y;
        hit.vNg.z[0] = Ng.z;
        vboolx valid_o = false;
        set(valid_o,0);
        return epilog(valid_o,hit);
      }
    };

#endif

#if 0

    struct Bezier1Intersector1
    {
      float depth_scale;
      LinearSpace3fa ray_space;

      __forceinline Bezier1Intersector1(const Ray& ray, const void* ptr) 
         : depth_scale(rsqrt(dot(ray.dir,ray.dir))), ray_space(frame(depth_scale*ray.dir).transposed()) {}

      template<typename Epilog>
      __forceinline bool intersect(Ray& ray,
                                   const Vec3fa& v0, const Vec3fa& v1, const Vec3fa& v2, const Vec3fa& v3, const int N,
                                   const Epilog& epilog) const
      {
        /* transform control points into ray space */
        STAT3(normal.trav_prims,1,1,1);
        Vec3fa w0 = xfmVector(ray_space,v0-ray.org); w0.w = v0.w;
        Vec3fa w1 = xfmVector(ray_space,v1-ray.org); w1.w = v1.w;
        Vec3fa w2 = xfmVector(ray_space,v2-ray.org); w2.w = v2.w;
        Vec3fa w3 = xfmVector(ray_space,v3-ray.org); w3.w = v3.w;
        BezierCurve3fa curve2D(w0,w1,w2,w3,0.0f,1.0f,4);
        
        /* evaluate the bezier curve */
        vboolx valid = vfloatx(step) < vfloatx(N);
        const Vec4vfx p0 = curve2D.eval0(valid,0,N);
        const Vec4vfx p1 = curve2D.eval1(valid,0,N);
        
        /* approximative intersection with cone */
        const Vec4vfx v = p1-p0;
        const Vec4vfx w = -p0;
        const vfloatx d0 = w.x*v.x + w.y*v.y;
        const vfloatx d1 = v.x*v.x + v.y*v.y;
        const vfloatx u = clamp(d0*rcp(d1),vfloatx(zero),vfloatx(one));
        const Vec4vfx p = p0 + u*v;
        const vfloatx t = p.z*depth_scale;
        const vfloatx d2 = p.x*p.x + p.y*p.y; 
        const vfloatx r = p.w;
        const vfloatx r2 = r*r;
        valid &= d2 <= r2 & vfloatx(ray.tnear) < t & t < vfloatx(ray.tfar);

        /* update hit information */
         bool ishit = false;
        if (unlikely(any(valid))) {
          BezierHit<VSIZEX> hit(valid,u,0.0f,t,0,N,v0,v1,v2,v3);
          ishit |= epilog(valid,hit);
        }

        if (unlikely(VSIZEX < N)) 
        {
          /* process SIMD-size many segments per iteration */
          for (int i=VSIZEX; i<N; i+=VSIZEX)
          {
            /* evaluate the bezier curve */
            vboolx valid = vintx(i)+vintx(step) < vintx(N);
            const Vec4vfx p0 = curve2D.eval0(valid,i,N);
            const Vec4vfx p1 = curve2D.eval1(valid,i,N);
            
            /* approximative intersection with cone */
            const Vec4vfx v = p1-p0;
            const Vec4vfx w = -p0;
            const vfloatx d0 = w.x*v.x + w.y*v.y;
            const vfloatx d1 = v.x*v.x + v.y*v.y;
            const vfloatx u = clamp(d0*rcp(d1),vfloatx(zero),vfloatx(one));
            const Vec4vfx p = p0 + u*v;
            const vfloatx t = p.z*depth_scale;
            const vfloatx d2 = p.x*p.x + p.y*p.y; 
            const vfloatx r = p.w;
            const vfloatx r2 = r*r;
            valid &= d2 <= r2 & vfloatx(ray.tnear) < t & t < vfloatx(ray.tfar);

             /* update hit information */
            if (unlikely(any(valid))) {
              BezierHit<VSIZEX> hit(valid,u,0.0f,t,i,N,v0,v1,v2,v3);
              ishit |= epilog(valid,hit);
            }
          }
        }
        return ishit;
      }
    };
#endif

    template<int K>
    struct Bezier1IntersectorK
    {
      vfloat<K> depth_scale;
      LinearSpace3fa ray_space[K];

      __forceinline Bezier1IntersectorK(const vbool<K>& valid, const RayK<K>& ray) 
      {
        int mask = movemask(valid);
        depth_scale = rsqrt(dot(ray.dir,ray.dir));
        while (mask) {
          size_t k = __bscf(mask);
          ray_space[k] = frame(depth_scale[k]*Vec3fa(ray.dir.x[k],ray.dir.y[k],ray.dir.z[k])).transposed();
        }
      }

      __forceinline Bezier1IntersectorK (const RayK<K>& ray, size_t k)
      {
        Vec3fa ray_dir = Vec3fa(ray.dir.x[k],ray.dir.y[k],ray.dir.z[k]);
        depth_scale[k] = rsqrt(dot(ray_dir,ray_dir));
        ray_space  [k] = frame(depth_scale[k]*ray_dir).transposed();
      }
      
      template<typename Epilog>
      __forceinline bool intersect(RayK<K>& ray, size_t k,
                                   const Vec3fa& v0, const Vec3fa& v1, const Vec3fa& v2, const Vec3fa& v3, const int N,
                                   const Epilog& epilog) const
      {
        /* load ray */
        const Vec3fa ray_org(ray.org.x[k],ray.org.y[k],ray.org.z[k]);
        const Vec3fa ray_dir(ray.dir.x[k],ray.dir.y[k],ray.dir.z[k]);
        const float ray_tnear = ray.tnear[k];
        const float ray_tfar  = ray.tfar [k];
        
        /* transform control points into ray space */
        Vec3fa w0 = xfmVector(ray_space[k],v0-ray_org); w0.w = v0.w;
        Vec3fa w1 = xfmVector(ray_space[k],v1-ray_org); w1.w = v1.w;
        Vec3fa w2 = xfmVector(ray_space[k],v2-ray_org); w2.w = v2.w;
        Vec3fa w3 = xfmVector(ray_space[k],v3-ray_org); w3.w = v3.w;
        BezierCurve3fa curve2D(w0,w1,w2,w3,0.0f,1.0f,4);
        
        /* process SIMD-size many segments per iteration */
        bool ishit = false;
        for (int i=0; i<N; i+=VSIZEX)
        {
          /* evaluate the bezier curve */
          vboolx valid = vintx(i)+vintx(step) < vintx(N);
          const Vec4vfx p0 = curve2D.eval0(valid,i,N);
          const Vec4vfx p1 = curve2D.eval1(valid,i,N);
          
          /* approximative intersection with cone */
          const Vec4vfx v = p1-p0;
          const Vec4vfx w = -p0;
          const vfloatx d0 = w.x*v.x + w.y*v.y;
          const vfloatx d1 = v.x*v.x + v.y*v.y;
          const vfloatx u = clamp(d0*rcp(d1),vfloatx(zero),vfloatx(one));
          const Vec4vfx p = p0 + u*v;
          const vfloatx t = p.z*depth_scale[k];
          const vfloatx d2 = p.x*p.x + p.y*p.y; 
          const vfloatx r = p.w;
          const vfloatx r2 = r*r;
          valid &= d2 <= r2 & vfloatx(ray_tnear) < t & t < vfloatx(ray_tfar);
          if (likely(none(valid))) continue;
        
          /* update hit information */
          BezierHit<VSIZEX> hit(valid,u,0.0f,t,i,N,v0,v1,v2,v3);
          ishit |= epilog(valid,hit);
        }
        return ishit;
      }
    };
  }
}
