/*!
  \file md_force.cxx
  \brief Routines to compute MD forces on particles
  \author Y. Nakayama
  \date 2006/06/27
  \version 1.1
 */

#include "md_force.h"

double *Hydro_force;
double *Hydro_force_new;

#define Cell_length 16

void Calc_f_Lennard_Jones_shear_cap_primitive_lnk(Particle *p
						  ,void (*distance0_func)(const double *x1,const double *x2,double &r12,double *x12)
						  ,const double cap
						  ){
  // Particle 変数の f に 
  // !! += 
  //で足す. f の初期値 が正しいと仮定している!!
  const double pair_cutoff = (!SW_PATCHY ? A_R_cutoff * LJ_dia : PATCHY_A_R_cutoff * SIGMA);
  double r_ij_vec[DIM] = {0.0, 0.0, 0.0};
  double r_ij = 0.0;
  double shear_stress[2] = {0.0, 0.0};
  
  // List Constructor
  int i,j;
  int *lscl;
  lscl = alloc_1d_int(Particle_Number);
  int lc[DIM];
  double lc_r[DIM];
  int mc[DIM];
  int lcyz,lcxyz;
  int ic[DIM],cn;
  lc[0] = int (NX / Cell_length);
  lc[1] = int (NY / Cell_length);
  lc[2] = int (NZ / Cell_length);
  for(int d=0; d < DIM; d++ ){ 
      lc_r[d] = Cell_length * DX;
  }
  lcyz = lc[1]*lc[2];
  lcxyz = lc[0]*lcyz;
  
  int *head;
  head = alloc_1d_int(lcxyz);
  
#pragma omp parallel for 
  for(int d=0; d < lcxyz; d++) head[d] = -1;
  for(int n=0;n<Particle_Number ; n++){
      Particle *p_n = &p[n];
      for(int d=0; d < DIM; d++ ){ 
	  mc[d] = int( (*p_n).x[d] / lc_r[d] ) ;
      }
      cn = mc[0]*lcyz+mc[1]*lc[2]+mc[2];
      lscl[n] = head[cn];
      head[cn] = n;
  }
  
  for (ic[0]= 0 ; ic[0] < lc[0] ; ic[0]++){ 
      for (ic[1]= 0 ; ic[1] < lc[1] ; ic[1]++){
	  for (ic[2]= 0 ; ic[2] < lc[2] ; ic[2]++){
	      cn = ic[0]*lcyz+ic[1]*lc[2]+ic[2];
// Scan the neighbor cells (including itself) of cell c
	      int icl[DIM],icls[DIM];
	      for (icl[0]= ic[0]-1 ; icl[0] <= ic[0]+1 ; icl[0]++){ 
		  for (icl[1]= ic[1]-1 ; icl[1] <= ic[1]+1 ; icl[1]++){
		      for (icl[2]= ic[2]-1 ; icl[2] <= ic[2]+1 ; icl[2]++){
// Consider periodic condition
			  for(int d=0; d < DIM; d++ ){ 
			      icls[d] = icl[d];
			      if(icls[d] < 0){
				  icls[d] = lc[d] -1;
			      }
			      if(icls[d] > (lc[d]-1)){
				  icls[d] = 0;
			      }
			  }
// Calculate the scalar cell index of the neighbor cell
			  int cl = ((icls[0]+lc[0])%lc[0])*lcyz+((icls[1]+lc[1])%lc[1])*lc[2]+((icls[2]+lc[2])%lc[2]);
// Scan atom i in cell c
			  i = head[cn];
			  while (i != -1){
			      j = head[cl];
		double i_dir[DIM] = {0.0, 0.0, 0.0};

		if(SW_PATCHY) rigid_body_rotation(i_dir, PATCHY_AXIS, p[i].q, BODY2SPACE);

		
			      while (j != -1){
                                if (i > j && !rigid_chain(i,j) && !obstacle_chain(p[i].spec,p[j].spec)) {
				      distance0_func( p[i].x, p[j].x, r_ij, r_ij_vec);
		    
		    if (r_ij < pair_cutoff){
		      double dmy_r, dmy_o;
		      double j_dir[DIM] = {0.0, 0.0, 0.0};
		      double n_ij_vec[DIM] = {0.0, 0.0, 0.0};
		      dmy_r = dmy_o = 0.0;
		      
		      if(SW_PATCHY){
			rigid_body_rotation(j_dir, PATCHY_AXIS, p[j].q, BODY2SPACE);
			n_ij_vec[0] = (j_dir[0] - i_dir[0]);
			n_ij_vec[1] = (j_dir[1] - i_dir[1]);
			n_ij_vec[2] = (j_dir[2] - i_dir[2]);

			patchy_janus_f(dmy_r, dmy_o, r_ij,
				       n_ij_vec[0]*r_ij_vec[0] + n_ij_vec[1]*r_ij_vec[1] + n_ij_vec[2]*r_ij_vec[2],
				       SIGMA);
			dmy_r = MIN(cap/r_ij,dmy_r);
		      }else{
			dmy_r = MIN(cap/r_ij,Lennard_Jones_f( r_ij , LJ_dia));			
		      }
		      
		      {
			//spherical particle forces
			double dmy_fi[DIM] = {0.0, 0.0, 0.0};
					  for(int d=0; d < DIM; d++ ){ 
			  dmy_fi[d] = (dmy_r)*(-r_ij_vec[d]) + (dmy_o)*(-n_ij_vec[d]);
			  
			  p[i].fr[d] += dmy_fi[d];
			  p[j].fr[d] -= dmy_fi[d];			  
			}

			//spherical particle torques
			double dmy_ti[DIM] = {0.0, 0.0, 0.0};
			double dmy_tj[DIM] = {0.0, 0.0, 0.0};

			if(SW_PATCHY){
			  dmy_ti[0] = (dmy_o) * (r_ij_vec[1]*i_dir[2] - r_ij_vec[2]*i_dir[1]);
			  dmy_ti[1] = (dmy_o) * (r_ij_vec[2]*i_dir[0] - r_ij_vec[0]*i_dir[2]);
			  dmy_ti[2] = (dmy_o) * (r_ij_vec[0]*i_dir[1] - r_ij_vec[1]*i_dir[0]);		     
			  
			  dmy_tj[0] = (-dmy_o) * (r_ij_vec[1]*j_dir[2] - r_ij_vec[2]*j_dir[1]);
			  dmy_tj[1] = (-dmy_o) * (r_ij_vec[2]*j_dir[0] - r_ij_vec[0]*j_dir[2]);
			  dmy_tj[2] = (-dmy_o) * (r_ij_vec[0]*j_dir[1] - r_ij_vec[1]*j_dir[0]);
			}
			for(int d = 0; d < DIM; d++){
			  p[i].torque_r[d] += dmy_ti[d];
			  p[j].torque_r[d] += dmy_tj[d];			  
			}

			//stress
			shear_stress[0] += (dmy_fi[0] * r_ij_vec[1]);
			shear_stress[1] += ((dmy_ti[2] + dmy_tj[2])/2.0);
			
					  }
				      }
				  }
				  j = lscl[j];
			      }
			      i = lscl[i];
			  }
		      }
		  }
	      }
	  }
      }
  }

  dev_shear_stress_lj  += shear_stress[0];
  dev_shear_stress_rot += shear_stress[1];
  
  free_1d_int(lscl);
  free_1d_int(head); 
}

void Calc_f_Lennard_Jones_shear_cap_primitive(Particle *p
					      ,void (*distance0_func)(const double *x1,const double *x2,double &r12,double *x12)
					      ,const double cap
					      ){
  // Particle $BJQ?t$N(B f $B$K(B 
  // !! += 
  //$B$GB-$9(B. f $B$N=i4|CM(B $B$,@5$7$$$H2>Dj$7$F$$$k(B!!
  const double pair_cutoff = (!SW_PATCHY ? A_R_cutoff * LJ_dia : PATCHY_A_R_cutoff * SIGMA);

  double shear_stress[2] = {0.0, 0.0};
  
  for(int n=0;n<Particle_Number ; n++){
    Particle *p_n = &p[n];
    double n_dir[DIM] = {0.0, 0.0, 0.0};

    if(SW_PATCHY) rigid_body_rotation(n_dir, PATCHY_AXIS, p_n->q, BODY2SPACE);
    
    for(int m=n+1; m < Particle_Number ; m++){
      double m_dir[DIM] = {0.0, 0.0, 0.0};
      double n_ij_vec[DIM] = {0.0, 0.0, 0.0};
      double r_ij_vec[DIM] = {0.0, 0.0, 0.0};
      double r_ij = 0.0;

      distance0_func( (*p_n).x, p[m].x, r_ij, r_ij_vec);

      if(r_ij < pair_cutoff && !rigid_chain(n,m) && !obstacle_chain(p[n].spec,p[m].spec)){
	double dmy_r, dmy_o;
	dmy_r = dmy_o = 0.0;

	if(SW_PATCHY){
	  rigid_body_rotation(m_dir, PATCHY_AXIS, p[m].q, BODY2SPACE);
	  n_ij_vec[0] = (m_dir[0] - n_dir[0]);
	  n_ij_vec[1] = (m_dir[1] - n_dir[1]);
	  n_ij_vec[2] = (m_dir[2] - n_dir[2]);
	  
	  patchy_janus_f(dmy_r, dmy_o, r_ij,
			 n_ij_vec[0]*r_ij_vec[0] + n_ij_vec[1]*r_ij_vec[1] + n_ij_vec[2]*r_ij_vec[2],
			 SIGMA
			 );	  
	}else{
	  dmy_r = MIN(cap/r_ij,Lennard_Jones_f( r_ij , LJ_dia));
	  dmy_o = 0.0;
	}

	{
	  //forces
	  double dmy_fn[DIM] = {0.0, 0.0, 0.0};
	  for(int d=0; d < DIM; d++ ){ 
	    dmy_fn[d] = (dmy_r) * (-r_ij_vec[d]) + (dmy_o) * (-n_ij_vec[d]);
	    
	    (*p_n).fr[d] += dmy_fn[d];
	    p[m].fr[d]   -= dmy_fn[d];
	  }

	  //torques	  
	  double dmy_tn[DIM] = {0.0, 0.0, 0.0};
	  double dmy_tm[DIM] = {0.0, 0.0, 0.0};
	  if(SW_PATCHY){	
	    dmy_tn[0] = (dmy_o)*(r_ij_vec[1]*n_dir[2] - r_ij_vec[2]*n_dir[1]);
	    dmy_tn[1] = (dmy_o)*(r_ij_vec[2]*n_dir[0] - r_ij_vec[0]*n_dir[2]);
	    dmy_tn[2] = (dmy_o)*(r_ij_vec[0]*n_dir[1] - r_ij_vec[1]*n_dir[0]);
	    
	    dmy_tm[0] = (-dmy_o)*(r_ij_vec[1]*m_dir[2] - r_ij_vec[2]*m_dir[1]);
	    dmy_tm[1] = (-dmy_o)*(r_ij_vec[2]*m_dir[0] - r_ij_vec[0]*m_dir[2]);
	    dmy_tm[2] = (-dmy_o)*(r_ij_vec[0]*m_dir[1] - r_ij_vec[1]*m_dir[0]);
	  }
	  for(int d = 0; d < DIM; d++){
	    (*p_n).torque_r[d] += dmy_tn[d];
	    p[m].torque_r[d]   += dmy_tm[d];
	  }

	  //stress
	  shear_stress[0] += (dmy_fn[0] * r_ij_vec[1]);
	  shear_stress[1] += ((dmy_tn[2] + dmy_tm[2])/2.0);

	}
      }
    }
  }

  dev_shear_stress_lj  += shear_stress[0];
  dev_shear_stress_rot += shear_stress[1];
}

void Add_f_gravity(Particle *p){
  static const double Gravity_on_fluid 
    = G*RHO * 4./3.*M_PI * SQ(RADIUS)* RADIUS;
   // Particle $BJQ?t$N(B f $B$K(B 
  // !! += 
  //$B$GB-$9(B. f $B$N=i4|CM(B $B$,@5$7$$$H2>Dj$7$F$$$k(B!!
#pragma omp parallel for  
  for(int n = 0; n < Particle_Number ; n++){
    p[n].fr[G_direction] -= Gravity_on_fluid * (MASS_RATIOS[p[n].spec] -1.0); 
  }
}
void Calc_f_slip_correct_precision(Particle *p, double const* const* u, const CTime &jikan){
    static const double dmy0 = DX3*RHO; 
    double dmy = dmy0 /jikan.dt_fluid; 
    int *nlattice;
    nlattice = Ns;

    double xp[DIM];
    int x_int[DIM];
    double residue[DIM];
    int sw_in_cell;
    double force[DIM];
    double torque[DIM];
    int r_mesh[DIM];
    double r[DIM];
    double dmy_fp[DIM];
    double x[DIM];
    double dmyR;
    double dmy_phi;
    int pspec;
#pragma omp parallel for \
  private(xp,x_int,residue,sw_in_cell,force,torque,r_mesh,r,dmy_fp,x,dmyR,dmy_phi,pspec) 
    for(int n = 0; n < Particle_Number; n++){
      for (int d = 0; d < DIM; d++) {
	xp[d] = p[n].x[d];

        force[d] = torque[d] = 0.0;
      }
      
      sw_in_cell = Particle_cell(xp, DX, x_int, residue);
      sw_in_cell = 1;
      
      for(int mesh=0; mesh < NP_domain; mesh++){
	Relative_coord(Sekibun_cell[mesh], x_int, residue, sw_in_cell, nlattice, DX, r_mesh, r);
	double x[DIM];
	for(int d=0;d<DIM;d++){
	  x[d] = r_mesh[d] * DX;
	}
	dmyR = Distance(x, xp);
	dmy_phi= Phi(dmyR, RADIUS);
	
	int im = (r_mesh[0] * NY * NZ_) + (r_mesh[1] * NZ_) + r_mesh[2];
	for(int d=0; d < DIM; d++ ){
	  dmy_fp[d] = u[d][im]*dmy_phi;
	  force[d] += dmy_fp[d];
	}
	{// torque
	  torque[0] += (r[1] * dmy_fp[2] - r[2] * dmy_fp[1]);
	  torque[1] += (r[2] * dmy_fp[0] - r[0] * dmy_fp[2]);
	  torque[2] += (r[0] * dmy_fp[1] - r[1] * dmy_fp[0]);
	}
      }// mesh
      

      pspec = p[n].spec;
      for(int d = 0; d < DIM; d++){
	p[n].f_slip[d] = (dmy * force[d]);
        p[n].torque_slip[d] = (dmy * torque[d]);
      }
    }//Particle_number
}
void Calc_f_hydro_correct_precision(Particle *p, double const* phi_sum, double const* const* u,
                                    const CTime &jikan){
    static const double dmy0 = -DX3*RHO;
    double dmy = dmy0 /jikan.dt_fluid; 
    
    int *nlattice;
    //if (SW_EQ == Shear_Navier_Stokes){
    //nlattice = Ns_shear;
    //}else {
    nlattice = Ns;
    //}
    double xp[DIM],vp[DIM],omega_p[DIM];
    int x_int[DIM];
    double residue[DIM];
    int sw_in_cell;
    double force[DIM];
    double torque[DIM];
    int r_mesh[DIM];
    double r[DIM];
    double dmy_fp[DIM];
    double x[DIM];
    double dmyR;
    double dmy_phi;
    double v_rot[DIM];
    int pspec;

    double forceg[DIM];
    double torqueg[DIM];
    int rigidID;
    // initialize forceGs and torqueGs
    for(int rigidID=0; rigidID<Rigid_Number; rigidID++){
      for(int d=0; d<DIM; d++){
        forceGs[rigidID][d] = 0.0;
        torqueGs[rigidID][d] = 0.0;
      }
    }

#pragma omp parallel for \
  private(xp,vp,omega_p,x_int,residue,sw_in_cell,force,torque,r_mesh,r,dmy_fp,x,dmyR,dmy_phi,v_rot,pspec, \
          rigidID, forceg, torqueg) 
    for(int n = 0; n < Particle_Number ; n++){
	//double xp[DIM],vp[DIM],omega_p[DIM];
	//int x_int[DIM];
	//double residue[DIM];
	if(SW_PT == rigid) rigidID = Particle_RigidID[n];
	for (int d = 0; d < DIM; d++) {
	    xp[d] = p[n].x[d];
	    vp[d] = p[n].v[d];
	    omega_p[d] = p[n].omega[d];

	    force[d] = torque[d] = 0.0;
            forceg[d] = torqueg[d] = 0.0;
	}

	sw_in_cell 
	    = Particle_cell(xp, DX, x_int, residue);// {1,0} $B$,JV$C$F$/$k(B
	sw_in_cell = 1;
	for(int mesh=0; mesh < NP_domain; mesh++){
	    Relative_coord(Sekibun_cell[mesh], x_int, residue, sw_in_cell, nlattice, DX, r_mesh, r);
	    for(int d=0;d<DIM;d++){
		x[d] = r_mesh[d] * DX;
		//dmyR += SQ(r[d]);
	    }
	    //dmyR = sqrt(dmyR); // vesion2.10 needs this value
	    dmyR = Distance(x, xp); // vesion2.00 needs this value
	    Angular2v(omega_p, r, v_rot);

	    int im = (r_mesh[0] * NY * NZ_) + (r_mesh[1] * NZ_) + r_mesh[2];
            dmy_phi = Phi(dmyR, RADIUS) / MAX(phi_sum[im], 1.0);
	    for(int d=0; d < DIM; d++ ){ 
              dmy_fp[d] = ((vp[d]+v_rot[d]) - u[d][im])*dmy_phi;
              force[d] += dmy_fp[d];
	    }
	    {// torque
              torque[0] += (r[1] * dmy_fp[2] - r[2] * dmy_fp[1]);
              torque[1] += (r[2] * dmy_fp[0] - r[0] * dmy_fp[2]);
              torque[2] += (r[0] * dmy_fp[1] - r[1] * dmy_fp[0]);
	    }
            
            if(SW_PT == rigid){
              for(int d = 0; d < DIM; d++) forceg[d] += dmy_fp[d];
              torqueg[0] += ( (GRvecs[n][1] + r[1]) * dmy_fp[2] - (GRvecs[n][2] + r[2]) * dmy_fp[1] );
              torqueg[1] += ( (GRvecs[n][2] + r[2]) * dmy_fp[0] - (GRvecs[n][0] + r[0]) * dmy_fp[2] );
              torqueg[2] += ( (GRvecs[n][0] + r[0]) * dmy_fp[1] - (GRvecs[n][1] + r[1]) * dmy_fp[0] );
            }
        }// mesh

        pspec = p[n].spec;

        for(int d = 0; d < DIM; d++){
          p[n].f_hydro[d] = (dmy * force[d]);
          p[n].f_slip[d] = 0.0;
	}
	if(ROTATION){
	  for(int d = 0; d < DIM; d++){
	    p[n].torque_hydro[d] = (dmy * torque[d]);
	    p[n].torque_slip[d] = 0.0;
	  }
	}
	if(SW_PT == rigid){
          for(int d=0; d<DIM; d++){
            #pragma omp atomic
            forceGs[rigidID][d] += dmy * forceg[d];

            #pragma omp atomic
            torqueGs[rigidID][d] += dmy * torqueg[d];
          }
	}
    }//Particle_Number
}

void Calc_f_hydro_correct_precision_OBL(Particle *p, double const* phi_sum, double const* const* u, const CTime &jikan){
    static const double dmy0 = -DX3*RHO;
    double dmy = dmy0 /jikan.dt_fluid; 
    
    int *nlattice;
    nlattice = Ns;
    
    double xp[DIM],vp[DIM],omega_p[DIM];
    int x_int[DIM];
    double residue[DIM];
    int sw_in_cell;
    double force[DIM];
    double torque[DIM];
    int r_mesh[DIM];
    double r[DIM];
    double dmy_fp[DIM];
    double x[DIM];
    double dmyR;
    double dmy_phi;
    double dmy_rhop;
    double dmy_ry;
    double v_rot[DIM];
    double volume[Particle_Number];
    double Itrace[Particle_Number];
    int sign;
    int im;
    double sum_force = 0.0;
    double sum_volume = 0.0;
    
    double forceg[DIM];
    double torqueg[DIM];
    double dVg[Rigid_Number][DIM];
    double dWg[Rigid_Number][DIM];
    // initialize forceGs and torqueGs
    for(int rigidID=0; rigidID<Rigid_Number; rigidID++){
      for(int d=0; d<DIM; d++){
        forceGs[rigidID][d] = 0.0;
        torqueGs[rigidID][d] = 0.0;
      }
    }
    
    Reset_phi(Hydro_force);
    Reset_phi(Hydro_force_new);
#pragma omp parallel for \
  private(xp,vp,omega_p,x_int,residue,sw_in_cell,force,torque,r_mesh,r,dmy_fp,x,\
          dmyR,dmy_phi,dmy_rhop,dmy_ry,v_rot,sign,im,forceg,torqueg) 
    for(int n = 0; n < Particle_Number ; n++){
      int rigidID;
      dmy_rhop = RHO_particle[p[n].spec];
      if(SW_PT == rigid) rigidID = Particle_RigidID[n];
      
      for (int d = 0; d < DIM; d++){
        xp[d] = p[n].x[d];
        vp[d] = p[n].v[d];
        omega_p[d] = p[n].omega[d];
        
        force[d] = torque[d] = 0.0;
        forceg[d] = torqueg[d] = 0.0;
      }
      
      volume[n] = 0.0;
      Itrace[n] = 0.0;
      sw_in_cell 
        = Particle_cell(xp, DX, x_int, residue);// {1,0} $B$,JV$C$F$/$k(B
      sw_in_cell = 1;
      
      for(int mesh=0; mesh < NP_domain; mesh++){
        sign = Relative_coord_check_stepover_Y(Sekibun_cell[mesh], x_int, residue, 
                                               sw_in_cell, nlattice, DX, r_mesh, r);
        dmyR = 0.;
        for(int d=0;d<DIM;d++){
          x[d] = r_mesh[d] * DX;
          dmyR += SQ(r[d]);
        }
        im = (r_mesh[0]*NY*NZ_ + r_mesh[1]*NZ_ + r_mesh[2]);
	
        dmyR = sqrt(dmyR);
        dmy_phi= Phi(dmyR, RADIUS) / MAX(phi_sum[im], 1.0);

        Angular2v(omega_p, r, v_rot);

        for(int d=0; d < DIM; d++ ){ 
          if (!(d==0)) {
            dmy_fp[d] =	((vp[d]+v_rot[d]) - u[d][im])*dmy_phi;
          } else {
            dmy_fp[d] =	((vp[d] - sign*Shear_rate_eff*L_particle[1] + v_rot[d]) 
                         - u[d][im])*dmy_phi;
          }
          force[d] += dmy_fp[d];
        }
        {// torque
          torque[0] += (r[1] * dmy_fp[2] - r[2] * dmy_fp[1]);
          torque[1] += (r[2] * dmy_fp[0] - r[0] * dmy_fp[2]);
          torque[2] += (r[0] * dmy_fp[1] - r[1] * dmy_fp[0]);
        }
        if(SW_PT == rigid){
          for(int d=0; d<DIM; d++) forceg[d] += dmy_fp[d];
          torqueg[0] += ( (GRvecs[n][1] + r[1]) * dmy_fp[2] - (GRvecs[n][2] + r[2]) * dmy_fp[1] );
          torqueg[1] += ( (GRvecs[n][2] + r[2]) * dmy_fp[0] - (GRvecs[n][0] + r[0]) * dmy_fp[2] );
          torqueg[2] += ( (GRvecs[n][0] + r[0]) * dmy_fp[1] - (GRvecs[n][1] + r[1]) * dmy_fp[0] );
        }
	
        dmy_ry = (r_mesh[1] + sign*L_particle[1]);
#pragma omp atomic
        Hydro_force[im] += dmy_fp[0]*dmy_ry*dmy_rhop;//viscocity
#pragma omp atomic
        Hydro_force_new[im] += dmy_fp[0]*dmy_ry*dmy_rhop;
        
        volume[n] += dmy_phi;
        Itrace[n] += dmy_phi*SQ(dmyR);
      }//mesh
      Itrace[n] *= 2.0/3.0;
      
      for(int d=0; d < DIM; d++ ){ 
        p[n].f_hydro[d] = (dmy * force[d]);
      }
      if(ROTATION){
        for(int d=0; d < DIM; d++ ){ 
          p[n].torque_hydro[d] = ( dmy * torque[d]);
        }
      }
      if(SW_PT == rigid){
        for(int d=0; d<DIM; d++){
#pragma omp atomic
          forceGs[rigidID][d] += dmy * forceg[d];
          
#pragma omp atomic
          torqueGs[rigidID][d] += dmy * torqueg[d];
        }
      }

      for(int mesh=0; mesh < NP_domain; mesh++){
        sign = Relative_coord_check_stepover_Y(Sekibun_cell[mesh], x_int, residue,
                                               sw_in_cell, nlattice, DX, r_mesh, r);
        dmyR = 0.;
        for(int d=0;d<DIM;d++){
          dmyR += SQ(r[d]);
        }
        dmyR = sqrt(dmyR);
        dmy_phi= Phi(dmyR, RADIUS);
        
        im = (r_mesh[0]*NY*NZ_ + r_mesh[1]*NZ_ + r_mesh[2]);
        dmy_ry = (r_mesh[1] + sign*L_particle[1]);
        double dmy_stress_p = p[n].momentum_depend_fr[0] / volume[n];

#pragma omp atomic
        Hydro_force[im] -= dmy_stress_p*dmy_ry*dmy_phi;//viscocity

        if(SW_PT != rigid){
          double dmy_stress_v = -RHO*force[0] / volume[n];
          double dmy_stress_w = -RHO*(torque[1]*r[2] - torque[2]*r[1])/Itrace[n];
#pragma omp atomic
          Hydro_force_new[im] += (dmy_stress_v + dmy_stress_w)*dmy_ry*dmy_phi;

        }
      }
      
# pragma omp critical
      {
        sum_force += force[0];
        sum_volume += volume[n];
      }
    }// Particle_Number
    sum_volume /= RHO;

    if(SW_PT == rigid){
#pragma omp parallel for 
      for(int rigidID = 0; rigidID < Rigid_Number; rigidID++){
        for(int d = 0; d < DIM; d++){
          dVg[rigidID][d] = jikan.dt_md * Rigid_IMasses[rigidID] * forceGs[rigidID][d];
          dWg[rigidID][d] = jikan.dt_md * (Rigid_IMoments[rigidID][d][0] * torqueGs[rigidID][0] +
                                           Rigid_IMoments[rigidID][d][1] * torqueGs[rigidID][1] +
                                           Rigid_IMoments[rigidID][d][2] * torqueGs[rigidID][2]);
        }
      }
    }
    
#pragma omp parallel for \
  private(xp,x_int,residue,sw_in_cell,r_mesh,r,x,dmyR,dmy_phi,dmy_ry,sign,im,dmy_rhop) 
    for(int n = 0; n < Particle_Number ; n++){
      int rigidID;
      dmy_rhop = RHO_particle[p[n].spec];
      for (int d = 0; d < DIM; d++) {
        xp[d] = p[n].x[d];
      }
      sw_in_cell = Particle_cell(xp, DX, x_int, residue);// {1,0} が返ってくる
      sw_in_cell = 1;
      
      for(int mesh=0; mesh < NP_domain; mesh++){
        sign = Relative_coord_check_stepover_Y(Sekibun_cell[mesh], x_int, residue, \
                                               sw_in_cell, nlattice, DX, r_mesh, r);

        dmyR = 0;
        for(int d=0;d<DIM;d++){
          x[d] = r_mesh[d] * DX;
          dmyR += SQ(r[d]);
        }
        dmyR= sqrt(dmyR);
        dmy_phi = Phi(dmyR, RADIUS);

        im = (r_mesh[0]*NY*NZ_ + r_mesh[1]*NZ_ + r_mesh[2]);
        dmy_ry = (r_mesh[1] + sign*L_particle[1]);
#pragma omp atomic	
        Hydro_force[im] -= sum_force*dmy_ry*dmy_phi/sum_volume;//viscocity
        
        if(SW_PT == rigid){
          rigidID = Particle_RigidID[n];
          double dmy_stress_v = dVg[rigidID][0];
          double dmy_stress_w = dWg[rigidID][1]*(GRvecs[n][2] + r[2]) - dWg[rigidID][2]*(GRvecs[n][1] + r[1]);
#pragma omp atomic
          Hydro_force_new[im] += (dmy_stress_v + dmy_stress_w)*dmy_ry*dmy_phi*dmy_rhop;
        }
      }
    }//Particle_Number
}
