// Copyright (c) 2009-2011, Tor M. Aamodt, Wilson W.L. Fung, George L. Yuan,
// Ali Bakhoda, Andrew Turner, Ivan Sham
// The University of British Columbia
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// Redistributions of source code must retain the above copyright notice, this
// list of conditions and the following disclaimer.
// Redistributions in binary form must reproduce the above copyright notice, this
// list of conditions and the following disclaimer in the documentation and/or
// other materials provided with the distribution.
// Neither the name of The University of British Columbia nor the names of its
// contributors may be used to endorse or promote products derived from this
// software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


#include "../libcuda/gpu-sim.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <signal.h>
#include "zlib.h"

// #include "mem_fetch.h"

#include <time.h>
// #include "gpu-cache.h"
// #include "gpu-misc.h"
// #include "delayqueue.h"
// #include "shader.h"
// #include "icnt_wrapper.h"
// #include "dram.h"
// #include "addrdec.h"
// #include "stat-tool.h"

#include "../src/cuda-sim/ptx-stats.h"
#include "../libcuda/abstract_hardware_model.h"
#include "../src/debug.h"
#include "../libcuda/gpgpusim_entrypoint.h"
#include "../src/cuda-sim/cuda-sim.h"
#include "../src/cuda-sim/ptx_ir.h"
// #include "../src/trace.h"
// #include "mem_latency_stat.h"
// #include "power_stat.h"
// #include "visualizer.h"
// #include "stats.h"
// #include "../src/cuda-sim/cuda_device_runtime.h"

#include <stdio.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <string>

#define MAX(a,b) (((a)>(b))?(a):(b))


bool g_interactive_debugger_enabled=false;

unsigned long long  gpu_sim_cycle = 0;
unsigned long long  gpu_tot_sim_cycle = 0;


// performance counter for stalls due to congestion.
unsigned int gpu_stall_dramfull = 0;
unsigned int gpu_stall_icnt2sh = 0;
unsigned long long partiton_reqs_in_parallel = 0;
unsigned long long partiton_reqs_in_parallel_total = 0;
unsigned long long partiton_reqs_in_parallel_util = 0;
unsigned long long partiton_reqs_in_parallel_util_total = 0;
unsigned long long  gpu_sim_cycle_parition_util = 0;
unsigned long long  gpu_tot_sim_cycle_parition_util = 0;
unsigned long long partiton_replys_in_parallel = 0;
unsigned long long partiton_replys_in_parallel_total = 0;

tr1_hash_map<new_addr_type,unsigned> address_random_interleaving;

/* Clock Domains */

#define  CORE  0x01
#define  L2    0x02
#define  DRAM  0x04
#define  ICNT  0x08


#define MEM_LATENCY_STAT_IMPL





/////////////////////////////////////////////////////////////////////////////

void increment_x_then_y_then_z( dim3 &i, const dim3 &bound)
{
   i.x++;
   if ( i.x >= bound.x ) {
      i.x = 0;
      i.y++;
      if ( i.y >= bound.y ) {
         i.y = 0;
         if( i.z < bound.z )
            i.z++;
      }
   }
}


// TODO schi gpgpu_sim::gpgpu_sim( const gpgpu_sim_config &config )
gpgpu_sim::gpgpu_sim( const gpgpu_sim_config &config, CudaGPU *cuda_gpu )
    : gpgpu_t(config, cuda_gpu), m_config(config)
{
}


#if 0

void gpgpu_sim::launch( kernel_info_t *kinfo )
{
   unsigned cta_size = kinfo->threads_per_cta();
   if ( cta_size > m_shader_config->n_thread_per_shader ) {
      printf("Execution error: Shader kernel CTA (block) size is too large for microarch config.\n");
      printf("                 CTA size (x*y*z) = %u, max supported = %u\n", cta_size,
             m_shader_config->n_thread_per_shader );
      printf("                 => either change -gpgpu_shader argument in gpgpusim.config file or\n");
      printf("                 modify the CUDA source to decrease the kernel block size.\n");
      abort();
   }
   unsigned n=0;
   for(n=0; n < m_running_kernels.size(); n++ ) {
       if( (NULL==m_running_kernels[n]) || m_running_kernels[n]->done() ) {
           m_running_kernels[n] = kinfo;
           break;
       }
   }
   assert(n < m_running_kernels.size());
}

bool gpgpu_sim::can_start_kernel()
{
   for(unsigned n=0; n < m_running_kernels.size(); n++ ) {
       if( (NULL==m_running_kernels[n]) || m_running_kernels[n]->done() )
           return true;
   }
   return false;
}

bool gpgpu_sim::hit_max_cta_count() const {
   if (m_config.gpu_max_cta_opt != 0) {
      if( (gpu_tot_issued_cta + m_total_cta_launched) >= m_config.gpu_max_cta_opt )
          return true;
   }
   return false;
}

bool gpgpu_sim::kernel_more_cta_left(kernel_info_t *kernel) const {
    if(hit_max_cta_count())
       return false;

    if(kernel && !kernel->no_more_ctas_to_run())
        return true;

    return false;
}

bool gpgpu_sim::get_more_cta_left() const
{
   if(hit_max_cta_count())
      return false;

   for(unsigned n=0; n < m_running_kernels.size(); n++ ) {
       if( m_running_kernels[n] && !m_running_kernels[n]->no_more_ctas_to_run() )
           return true;
   }
   return false;
}

kernel_info_t *gpgpu_sim::select_kernel()
{
    if(m_running_kernels[m_last_issued_kernel] &&
        !m_running_kernels[m_last_issued_kernel]->no_more_ctas_to_run()) {
        unsigned launch_uid = m_running_kernels[m_last_issued_kernel]->get_uid();
        if(std::find(m_executed_kernel_uids.begin(), m_executed_kernel_uids.end(), launch_uid) == m_executed_kernel_uids.end()) {
            m_running_kernels[m_last_issued_kernel]->start_cycle = gpu_sim_cycle + gpu_tot_sim_cycle;
            m_executed_kernel_uids.push_back(launch_uid);
            m_executed_kernel_names.push_back(m_running_kernels[m_last_issued_kernel]->name());
        }
        return m_running_kernels[m_last_issued_kernel];
    }

    for(unsigned n=0; n < m_running_kernels.size(); n++ ) {
        unsigned idx = (n+m_last_issued_kernel+1)%m_config.max_concurrent_kernel;
        if( kernel_more_cta_left(m_running_kernels[idx]) ){
            m_last_issued_kernel=idx;
            m_running_kernels[idx]->start_cycle = gpu_sim_cycle + gpu_tot_sim_cycle;
            // record this kernel for stat print if it is the first time this kernel is selected for execution
            unsigned launch_uid = m_running_kernels[idx]->get_uid();
            assert(std::find(m_executed_kernel_uids.begin(), m_executed_kernel_uids.end(), launch_uid) == m_executed_kernel_uids.end());
            m_executed_kernel_uids.push_back(launch_uid);
            m_executed_kernel_names.push_back(m_running_kernels[idx]->name());

            return m_running_kernels[idx];
        }
    }
    return NULL;
}

unsigned gpgpu_sim::finished_kernel()
{
    // This should never be called now
    assert(0);
    if( m_finished_kernel.empty() )
        return 0;
    unsigned result = m_finished_kernel.front();
    m_finished_kernel.pop_front();
    return result;
}

void gpgpu_sim::set_kernel_done( kernel_info_t *kernel )
{
    unsigned uid = kernel->get_uid();
    // TODO schi
    // m_finished_kernel.push_back(uid);
    gem5CudaGPU->finishKernel(uid);
    std::vector<kernel_info_t*>::iterator k;
    for( k=m_running_kernels.begin(); k!=m_running_kernels.end(); k++ ) {
        if( *k == kernel ) {
            kernel->end_cycle = gpu_sim_cycle + gpu_tot_sim_cycle;
            *k = NULL;
            break;
        }
    }
    assert( k != m_running_kernels.end() );
}

void gpgpu_sim::stop_all_running_kernels(){
    std::vector<kernel_info_t *>::iterator k;
    for(k = m_running_kernels.begin(); k != m_running_kernels.end(); ++k){
        if(*k != NULL){ // If a kernel is active
            set_kernel_done(*k); // Stop the kernel
            assert(*k==NULL);
        }
    }
}

void set_ptx_warp_size(const struct core_config * warp_size);

// TODO schi gpgpu_sim::gpgpu_sim( const gpgpu_sim_config &config )
gpgpu_sim::gpgpu_sim( const gpgpu_sim_config &config, CudaGPU *cuda_gpu )
    : gpgpu_t(config, cuda_gpu), m_config(config)
{
    m_shader_config = &m_config.m_shader_config;
    m_memory_config = &m_config.m_memory_config;
    set_ptx_warp_size(m_shader_config);
    ptx_file_line_stats_create_exposed_latency_tracker(m_config.num_shader());

#ifdef GPGPUSIM_POWER_MODEL
        m_gpgpusim_wrapper = new gpgpu_sim_wrapper(config.g_power_simulation_enabled,config.g_power_config_name);
#endif

    m_shader_stats = new shader_core_stats(m_shader_config);
    m_memory_stats = new memory_stats_t(m_config.num_shader(),m_shader_config,m_memory_config);
    average_pipeline_duty_cycle = (float *)malloc(sizeof(float));
    active_sms=(float *)malloc(sizeof(float));
    m_power_stats = new power_stat_t(m_shader_config,average_pipeline_duty_cycle,active_sms,m_shader_stats,m_memory_config,m_memory_stats);

    gpu_sim_insn = 0;
    gpu_tot_sim_insn = 0;
    gpu_tot_issued_cta = 0;
    m_total_cta_launched = 0;
    gpu_deadlock = false;


    m_cluster = new simt_core_cluster*[m_shader_config->n_simt_clusters];
    for (unsigned i=0;i<m_shader_config->n_simt_clusters;i++)
        m_cluster[i] = new simt_core_cluster(this,i,m_shader_config,m_memory_config,m_shader_stats,m_memory_stats);

    m_memory_partition_unit = new memory_partition_unit*[m_memory_config->m_n_mem];
    m_memory_sub_partition = new memory_sub_partition*[m_memory_config->m_n_mem_sub_partition];
    for (unsigned i=0;i<m_memory_config->m_n_mem;i++) {
        m_memory_partition_unit[i] = new memory_partition_unit(i, m_memory_config, m_memory_stats);
        for (unsigned p = 0; p < m_memory_config->m_n_sub_partition_per_memory_channel; p++) {
            unsigned submpid = i * m_memory_config->m_n_sub_partition_per_memory_channel + p;
            m_memory_sub_partition[submpid] = m_memory_partition_unit[i]->get_sub_partition(p);
        }
    }

    icnt_wrapper_init();
    icnt_create(m_shader_config->n_simt_clusters,m_memory_config->m_n_mem_sub_partition);

    time_vector_create(NUM_MEM_REQ_STAT);
    fprintf(stdout, "GPGPU-Sim uArch: performance model initialization complete.\n");

    m_running_kernels.resize( config.max_concurrent_kernel, NULL );
    m_last_issued_kernel = 0;
    m_last_cluster_issue = m_shader_config->n_simt_clusters-1; // this causes first launch to use simt cluster 0
    *average_pipeline_duty_cycle=0;
    *active_sms=0;

    last_liveness_message_time = 0;

   //Jin: functional simulation for CDP
   m_functional_sim = false;
   m_functional_sim_kernel = NULL;
}

int gpgpu_sim::shared_mem_size() const
{
   return m_shader_config->gpgpu_shmem_size;
}

int gpgpu_sim::shared_mem_per_block() const
{
   return m_shader_config->gpgpu_shmem_per_block;
}

int gpgpu_sim::num_registers_per_core() const
{
   return m_shader_config->gpgpu_shader_registers;
}

int gpgpu_sim::num_registers_per_block() const
{
   return m_shader_config->gpgpu_registers_per_block;
}

int gpgpu_sim::wrp_size() const
{
   return m_shader_config->warp_size;
}

int gpgpu_sim::shader_clock() const
{
   return m_config.core_freq/1000;
}

void gpgpu_sim::set_prop( cudaDeviceProp *prop )
{
   m_cuda_properties = prop;
}

int gpgpu_sim::compute_capability_major() const
{
   return m_config.gpgpu_compute_capability_major;
}

int gpgpu_sim::compute_capability_minor() const
{
   return m_config.gpgpu_compute_capability_minor;
}

const struct cudaDeviceProp *gpgpu_sim::get_prop() const
{
   return m_cuda_properties;
}

enum divergence_support_t gpgpu_sim::simd_model() const
{
   return m_shader_config->model;
}

void gpgpu_sim_config::init_clock_domains(void )
{
   sscanf(gpgpu_clock_domains,"%lf:%lf:%lf:%lf",
          &core_freq, &icnt_freq, &l2_freq, &dram_freq);
   core_freq = core_freq MhZ;
   icnt_freq = icnt_freq MhZ;
   l2_freq = l2_freq MhZ;
   dram_freq = dram_freq MhZ;
   core_period = 1/core_freq;
   icnt_period = 1/icnt_freq;
   dram_period = 1/dram_freq;
   l2_period = 1/l2_freq;
   printf("GPGPU-Sim uArch: clock freqs: %lf:%lf:%lf:%lf\n",core_freq,icnt_freq,l2_freq,dram_freq);
   printf("GPGPU-Sim uArch: clock periods: %.20lf:%.20lf:%.20lf:%.20lf\n",core_period,icnt_period,l2_period,dram_period);
}

void gpgpu_sim::reinit_clock_domains(void)
{
   core_time = 0;
   dram_time = 0;
   icnt_time = 0;
   l2_time = 0;
}

bool gpgpu_sim::active()
{
    if (m_config.gpu_max_cycle_opt && (gpu_tot_sim_cycle + gpu_sim_cycle) >= m_config.gpu_max_cycle_opt)
       return false;
    if (m_config.gpu_max_insn_opt && (gpu_tot_sim_insn + gpu_sim_insn) >= m_config.gpu_max_insn_opt)
       return false;
    if (m_config.gpu_max_cta_opt && (gpu_tot_issued_cta >= m_config.gpu_max_cta_opt) )
       return false;
    if (m_config.gpu_deadlock_detect && gpu_deadlock)
       return false;
    for (unsigned i=0;i<m_shader_config->n_simt_clusters;i++)
       if( m_cluster[i]->get_not_completed()>0 )
           return true;;
    for (unsigned i=0;i<m_memory_config->m_n_mem;i++)
       if( m_memory_partition_unit[i]->busy()>0 )
           return true;;
    if( icnt_busy() )
        return true;
    if( get_more_cta_left() )
        return true;
    return false;
}

void gpgpu_sim::init()
{
    // run a CUDA grid on the GPU microarchitecture simulator
    gpu_sim_cycle = 0;
    gpu_sim_insn = 0;
    last_gpu_sim_insn = 0;
    m_total_cta_launched=0;
    partiton_reqs_in_parallel = 0;
    partiton_replys_in_parallel = 0;
    partiton_reqs_in_parallel_util = 0;
    gpu_sim_cycle_parition_util = 0;

    reinit_clock_domains();
    set_param_gpgpu_num_shaders(m_config.num_shader());
    for (unsigned i=0;i<m_shader_config->n_simt_clusters;i++)
       m_cluster[i]->reinit();
    m_shader_stats->new_grid();
    // initialize the control-flow, memory access, memory latency logger
    if (m_config.g_visualizer_enabled) {
        create_thread_CFlogger( m_config.num_shader(), m_shader_config->n_thread_per_shader, 0, m_config.gpgpu_cflog_interval );
    }
    shader_CTA_count_create( m_config.num_shader(), m_config.gpgpu_cflog_interval);
    if (m_config.gpgpu_cflog_interval != 0) {
       insn_warp_occ_create( m_config.num_shader(), m_shader_config->warp_size );
       shader_warp_occ_create( m_config.num_shader(), m_shader_config->warp_size, m_config.gpgpu_cflog_interval);
       shader_mem_acc_create( m_config.num_shader(), m_memory_config->m_n_mem, 4, m_config.gpgpu_cflog_interval);
       shader_mem_lat_create( m_config.num_shader(), m_config.gpgpu_cflog_interval);
       shader_cache_access_create( m_config.num_shader(), 3, m_config.gpgpu_cflog_interval);
       set_spill_interval (m_config.gpgpu_cflog_interval * 40);
    }

    if (g_network_mode)
       icnt_init();

    // McPAT initialization function. Called on first launch of GPU
#ifdef GPGPUSIM_POWER_MODEL
    if(m_config.g_power_simulation_enabled){
        init_mcpat(m_config, m_gpgpusim_wrapper, m_config.gpu_stat_sample_freq,  gpu_tot_sim_insn, gpu_sim_insn);
    }
#endif
}

void gpgpu_sim::update_stats() {
    m_memory_stats->memlatstat_lat_pw();
    gpu_tot_sim_cycle += gpu_sim_cycle;
    gpu_tot_sim_insn += gpu_sim_insn;
    gpu_tot_issued_cta += m_total_cta_launched;
    partiton_reqs_in_parallel_total += partiton_reqs_in_parallel;
    partiton_replys_in_parallel_total += partiton_replys_in_parallel;
    partiton_reqs_in_parallel_util_total += partiton_reqs_in_parallel_util;
    gpu_tot_sim_cycle_parition_util += gpu_sim_cycle_parition_util ;
    gpu_tot_occupancy += gpu_occupancy;

    gpu_sim_cycle = 0;
    partiton_reqs_in_parallel = 0;
    partiton_replys_in_parallel = 0;
    partiton_reqs_in_parallel_util = 0;
    gpu_sim_cycle_parition_util = 0;
    gpu_sim_insn = 0;
    m_total_cta_launched = 0;
    gpu_occupancy = occupancy_stats();
}

void gpgpu_sim::print_stats()
{
    ptx_file_line_stats_write_file();
    gpu_print_stat();

    if (g_network_mode) {
        printf("----------------------------Interconnect-DETAILS--------------------------------\n" );
        icnt_display_stats();
        icnt_display_overall_stats();
        printf("----------------------------END-of-Interconnect-DETAILS-------------------------\n" );
    }
}

void gpgpu_sim::deadlock_check()
{
   if (m_config.gpu_deadlock_detect && gpu_deadlock) {
      fflush(stdout);
      printf("\n\nGPGPU-Sim uArch: ERROR ** deadlock detected: last writeback core %u @ gpu_sim_cycle %u (+ gpu_tot_sim_cycle %u) (%u cycles ago)\n",
             gpu_sim_insn_last_update_sid,
             (unsigned) gpu_sim_insn_last_update, (unsigned) (gpu_tot_sim_cycle-gpu_sim_cycle),
             (unsigned) (gpu_sim_cycle - gpu_sim_insn_last_update ));
      unsigned num_cores=0;
      for (unsigned i=0;i<m_shader_config->n_simt_clusters;i++) {
         unsigned not_completed = m_cluster[i]->get_not_completed();
         if( not_completed ) {
             if ( !num_cores )  {
                 printf("GPGPU-Sim uArch: DEADLOCK  shader cores no longer committing instructions [core(# threads)]:\n" );
                 printf("GPGPU-Sim uArch: DEADLOCK  ");
                 m_cluster[i]->print_not_completed(stdout);
             } else if (num_cores < 8 ) {
                 m_cluster[i]->print_not_completed(stdout);
             } else if (num_cores >= 8 ) {
                 printf(" + others ... ");
             }
             num_cores+=m_shader_config->n_simt_cores_per_cluster;
         }
      }
      printf("\n");
      for (unsigned i=0;i<m_memory_config->m_n_mem;i++) {
         bool busy = m_memory_partition_unit[i]->busy();
         if( busy )
             printf("GPGPU-Sim uArch DEADLOCK:  memory partition %u busy\n", i );
      }
      if( icnt_busy() ) {
         printf("GPGPU-Sim uArch DEADLOCK:  iterconnect contains traffic\n");
         icnt_display_state( stdout );
      }
      printf("\nRe-run the simulator in gdb and use debug routines in .gdbinit to debug this\n");
      fflush(stdout);
      abort();
   }
}

void gpgpu_sim::set_cache_config(std::string kernel_name,  FuncCache cacheConfig )
{
	m_special_cache_config[kernel_name]=cacheConfig ;
}


// performance counter that are not local to one shader
unsigned gpgpu_sim::threads_per_core() const
{
   return m_shader_config->n_thread_per_shader;
}


///////////////////////////////////////////////////////////////////////////////////////////

//void gpgpu_sim::cycle()
//{
//   int clock_mask = next_clock_domain();
//
//   if (clock_mask & CORE ) {
//       // shader core loading (pop from ICNT into core) follows CORE clock
//      for (unsigned i=0;i<m_shader_config->n_simt_clusters;i++)
//         m_cluster[i]->icnt_cycle();
//   }
//    unsigned partiton_replys_in_parallel_per_cycle = 0;
//    if (clock_mask & ICNT) {
//        // pop from memory controller to interconnect
//        for (unsigned i=0;i<m_memory_config->m_n_mem_sub_partition;i++) {
//            mem_fetch* mf = m_memory_sub_partition[i]->top();
//            if (mf) {
//                unsigned response_size = mf->get_is_write()?mf->get_ctrl_size():mf->size();
//                if ( ::icnt_has_buffer( m_shader_config->mem2device(i), response_size ) ) {
//                    //if (!mf->get_is_write())
//                       mf->set_return_timestamp(gpu_sim_cycle+gpu_tot_sim_cycle);
//                    mf->set_status(IN_ICNT_TO_SHADER,gpu_sim_cycle+gpu_tot_sim_cycle);
//                    ::icnt_push( m_shader_config->mem2device(i), mf->get_tpc(), mf, response_size );
//                    m_memory_sub_partition[i]->pop();
//                    partiton_replys_in_parallel_per_cycle++;
//                } else {
//                    gpu_stall_icnt2sh++;
//                }
//            } else {
//               m_memory_sub_partition[i]->pop();
//            }
//        }
//    }
//    partiton_replys_in_parallel += partiton_replys_in_parallel_per_cycle;
//
//   if (clock_mask & DRAM) {
//      for (unsigned i=0;i<m_memory_config->m_n_mem;i++){
//         m_memory_partition_unit[i]->dram_cycle(); // Issue the dram command (scheduler + delay model)
//         // Update performance counters for DRAM
//         m_memory_partition_unit[i]->set_dram_power_stats(m_power_stats->pwr_mem_stat->n_cmd[CURRENT_STAT_IDX][i], m_power_stats->pwr_mem_stat->n_activity[CURRENT_STAT_IDX][i],
//                        m_power_stats->pwr_mem_stat->n_nop[CURRENT_STAT_IDX][i], m_power_stats->pwr_mem_stat->n_act[CURRENT_STAT_IDX][i], m_power_stats->pwr_mem_stat->n_pre[CURRENT_STAT_IDX][i],
//                        m_power_stats->pwr_mem_stat->n_rd[CURRENT_STAT_IDX][i], m_power_stats->pwr_mem_stat->n_wr[CURRENT_STAT_IDX][i], m_power_stats->pwr_mem_stat->n_req[CURRENT_STAT_IDX][i]);
//      }
//   }
//
//   // L2 operations follow L2 clock domain
//   unsigned partiton_reqs_in_parallel_per_cycle = 0;
//   if (clock_mask & L2) {
//       m_power_stats->pwr_mem_stat->l2_cache_stats[CURRENT_STAT_IDX].clear();
//      for (unsigned i=0;i<m_memory_config->m_n_mem_sub_partition;i++) {
//          //move memory request from interconnect into memory partition (if not backed up)
//          //Note:This needs to be called in DRAM clock domain if there is no L2 cache in the system
//    	  //In the worst case, we may need to push SECTOR_CHUNCK_SIZE requests, so ensure you have enough buffer for them
//          if ( m_memory_sub_partition[i]->full(SECTOR_CHUNCK_SIZE) ) {
//             gpu_stall_dramfull++;
//          } else {
//              mem_fetch* mf = (mem_fetch*) icnt_pop( m_shader_config->mem2device(i) );
//              m_memory_sub_partition[i]->push( mf, gpu_sim_cycle + gpu_tot_sim_cycle );
//              if(mf)
//            	  partiton_reqs_in_parallel_per_cycle++;
//          }
//          m_memory_sub_partition[i]->cache_cycle(gpu_sim_cycle+gpu_tot_sim_cycle);
//          m_memory_sub_partition[i]->accumulate_L2cache_stats(m_power_stats->pwr_mem_stat->l2_cache_stats[CURRENT_STAT_IDX]);
//       }
//   }
//   partiton_reqs_in_parallel += partiton_reqs_in_parallel_per_cycle;
//   if(partiton_reqs_in_parallel_per_cycle > 0){
//	   partiton_reqs_in_parallel_util += partiton_reqs_in_parallel_per_cycle;
//	   gpu_sim_cycle_parition_util++;
//   }
//
//   if (clock_mask & ICNT) {
//      icnt_transfer();
//   }
//
//   if (clock_mask & CORE) {
//      // L1 cache + shader core pipeline stages
//      m_power_stats->pwr_mem_stat->core_cache_stats[CURRENT_STAT_IDX].clear();
//      for (unsigned i=0;i<m_shader_config->n_simt_clusters;i++) {
//         if (m_cluster[i]->get_not_completed() || get_more_cta_left() ) {
//               m_cluster[i]->core_cycle();
//               *active_sms+=m_cluster[i]->get_n_active_sms();
//         }
//         // Update core icnt/cache stats for GPUWattch
//         m_cluster[i]->get_icnt_stats(m_power_stats->pwr_mem_stat->n_simt_to_mem[CURRENT_STAT_IDX][i], m_power_stats->pwr_mem_stat->n_mem_to_simt[CURRENT_STAT_IDX][i]);
//         m_cluster[i]->get_cache_stats(m_power_stats->pwr_mem_stat->core_cache_stats[CURRENT_STAT_IDX]);
//         m_cluster[i]->get_current_occupancy(gpu_occupancy.aggregate_warp_slot_filled, gpu_occupancy.aggregate_theoretical_warp_slots);
//
//      }
//      float temp=0;
//      for (unsigned i=0;i<m_shader_config->num_shader();i++){
//        temp+=m_shader_stats->m_pipeline_duty_cycle[i];
//      }
//      temp=temp/m_shader_config->num_shader();
//      *average_pipeline_duty_cycle=((*average_pipeline_duty_cycle)+temp);
//        //cout<<"Average pipeline duty cycle: "<<*average_pipeline_duty_cycle<<endl;
//
//
//      if( g_single_step && ((gpu_sim_cycle+gpu_tot_sim_cycle) >= g_single_step) ) {
//          raise(SIGTRAP); // Debug breakpoint
//      }
//	 gpu_sim_cycle++;
//
//      if( g_interactive_debugger_enabled )
//         gpgpu_debug();
//
//      // McPAT main cycle (interface with McPAT)
//#ifdef GPGPUSIM_POWER_MODEL
//      if(m_config.g_power_simulation_enabled){
//          mcpat_cycle(m_config, getShaderCoreConfig(), m_gpgpusim_wrapper, m_power_stats, m_config.gpu_stat_sample_freq, gpu_tot_sim_cycle, gpu_sim_cycle, gpu_tot_sim_insn, gpu_sim_insn);
//      }
//#endif
//
//      issue_block2core();
//
//      // Depending on configuration, invalidate the caches once all of threads are completed.
//      int all_threads_complete = 1;
//      if (m_config.gpgpu_flush_l1_cache) {
//         for (unsigned i=0;i<m_shader_config->n_simt_clusters;i++) {
//            if (m_cluster[i]->get_not_completed() == 0)
//                m_cluster[i]->cache_invalidate();
//            else
//               all_threads_complete = 0 ;
//         }
//      }
//
//      if(m_config.gpgpu_flush_l2_cache){
//          if(!m_config.gpgpu_flush_l1_cache){
//              for (unsigned i=0;i<m_shader_config->n_simt_clusters;i++) {
//                  if (m_cluster[i]->get_not_completed() != 0){
//                      all_threads_complete = 0 ;
//                      break;
//                  }
//              }
//          }
//
//         if (all_threads_complete && !m_memory_config->m_L2_config.disabled() ) {
//            printf("Flushed L2 caches...\n");
//            if (m_memory_config->m_L2_config.get_num_lines()) {
//               int dlc = 0;
//               for (unsigned i=0;i<m_memory_config->m_n_mem;i++) {
//                  dlc = m_memory_sub_partition[i]->flushL2();
//                  assert (dlc == 0); // TODO: need to model actual writes to DRAM here
//                  printf("Dirty lines flushed from L2 %d is %d\n", i, dlc  );
//               }
//            }
//         }
//      }
//
//      if (!(gpu_sim_cycle % m_config.gpu_stat_sample_freq)) {
//         time_t days, hrs, minutes, sec;
//         time_t curr_time;
//         time(&curr_time);
//         unsigned long long  elapsed_time = MAX(curr_time - g_simulation_starttime, 1);
//         if ( (elapsed_time - last_liveness_message_time) >= m_config.liveness_message_freq && DTRACE(LIVENESS) ) {
//            days    = elapsed_time/(3600*24);
//            hrs     = elapsed_time/3600 - 24*days;
//            minutes = elapsed_time/60 - 60*(hrs + 24*days);
//            sec = elapsed_time - 60*(minutes + 60*(hrs + 24*days));
//
//            unsigned long long active = 0, total = 0;
//            for (unsigned i=0;i<m_shader_config->n_simt_clusters;i++) {
//                m_cluster[i]->get_current_occupancy(active, total);
//            }
//            DPRINTF(LIVENESS, "uArch: inst.: %lld (ipc=%4.1f, occ=%0.4f\% [%llu / %llu]) sim_rate=%u (inst/sec) elapsed = %u:%u:%02u:%02u / %s",
//                   gpu_tot_sim_insn + gpu_sim_insn,
//                   (double)gpu_sim_insn/(double)gpu_sim_cycle,
//                   float(active)/float(total) * 100, active, total,
//                   (unsigned)((gpu_tot_sim_insn+gpu_sim_insn) / elapsed_time),
//                   (unsigned)days,(unsigned)hrs,(unsigned)minutes,(unsigned)sec,
//                   ctime(&curr_time));
//            fflush(stdout);
//            last_liveness_message_time = elapsed_time;
//         }
//         visualizer_printstat();
//         m_memory_stats->memlatstat_lat_pw();
//         if (m_config.gpgpu_runtime_stat && (m_config.gpu_runtime_stat_flag != 0) ) {
//            if (m_config.gpu_runtime_stat_flag & GPU_RSTAT_BW_STAT) {
//               for (unsigned i=0;i<m_memory_config->m_n_mem;i++)
//                  m_memory_partition_unit[i]->print_stat(stdout);
//               printf("maxmrqlatency = %d \n", m_memory_stats->max_mrq_latency);
//               printf("maxmflatency = %d \n", m_memory_stats->max_mf_latency);
//            }
//            if (m_config.gpu_runtime_stat_flag & GPU_RSTAT_SHD_INFO)
//               shader_print_runtime_stat( stdout );
//            if (m_config.gpu_runtime_stat_flag & GPU_RSTAT_L1MISS)
//               shader_print_l1_miss_stat( stdout );
//            if (m_config.gpu_runtime_stat_flag & GPU_RSTAT_SCHED)
//               shader_print_scheduler_stat( stdout, false );
//         }
//      }
//
//      if (!(gpu_sim_cycle % 50000)) {
//         // deadlock detection
//         if (m_config.gpu_deadlock_detect && gpu_sim_insn == last_gpu_sim_insn) {
//            gpu_deadlock = true;
//         } else {
//            last_gpu_sim_insn = gpu_sim_insn;
//         }
//      }
//      try_snap_shot(gpu_sim_cycle);
//      spill_log_to_file (stdout, 0, gpu_sim_cycle);
//
//#if (CUDART_VERSION >= 5000)
//      //launch device kernel
//      launch_one_device_kernel();
//#endif
//   }
//}


void shader_core_ctx::dump_warp_state( FILE *fout ) const
{
   fprintf(fout, "\n");
   fprintf(fout, "per warp functional simulation status:\n");
   for (unsigned w=0; w < m_config->max_warps_per_shader; w++ )
       m_warp[w].print(fout);
}


void gpgpu_sim::perf_memcpy_to_gpu( size_t dst_start_addr, size_t count )
{
    if (m_memory_config->m_perf_sim_memcpy) {
       assert (dst_start_addr % 32 == 0);

       for ( unsigned counter = 0; counter < count; counter += 32 ) {
           const unsigned wr_addr = dst_start_addr + counter;
           addrdec_t raw_addr;
           mem_access_sector_mask_t mask;
           mask.set(wr_addr % 128 / 32);
           m_memory_config->m_address_mapping.addrdec_tlx( wr_addr, &raw_addr );
           const unsigned partition_id = raw_addr.sub_partition / m_memory_config->m_n_sub_partition_per_memory_channel;
           m_memory_partition_unit[ partition_id ]->handle_memcpy_to_gpu( wr_addr, raw_addr.sub_partition, mask );
       }
    }
}

void gpgpu_sim::dump_pipeline( int mask, int s, int m ) const
{
/*
   You may want to use this function while running GPGPU-Sim in gdb.
   One way to do that is add the following to your .gdbinit file:

      define dp
         call g_the_gpu.dump_pipeline_impl((0x40|0x4|0x1),$arg0,0)
      end

   Then, typing "dp 3" will show the contents of the pipeline for shader core 3.
*/

   printf("Dumping pipeline state...\n");
   if(!mask) mask = 0xFFFFFFFF;
   for (unsigned i=0;i<m_shader_config->n_simt_clusters;i++) {
      if(s != -1) {
         i = s;
      }
      if(mask&1) m_cluster[m_shader_config->sid_to_cluster(i)]->display_pipeline(i,stdout,1,mask & 0x2E);
      if(s != -1) {
         break;
      }
   }
   if(mask&0x10000) {
      for (unsigned i=0;i<m_memory_config->m_n_mem;i++) {
         if(m != -1) {
            i=m;
         }
         printf("DRAM / memory controller %u:\n", i);
         if(mask&0x100000) m_memory_partition_unit[i]->print_stat(stdout);
         if(mask&0x1000000)   m_memory_partition_unit[i]->visualize();
         if(mask&0x10000000)   m_memory_partition_unit[i]->print(stdout);
         if(m != -1) {
            break;
         }
      }
   }
   fflush(stdout);
}

const struct shader_core_config * gpgpu_sim::getShaderCoreConfig()
{
   return m_shader_config;
}

const struct memory_config * gpgpu_sim::getMemoryConfig()
{
   return m_memory_config;
}

simt_core_cluster * gpgpu_sim::getSIMTCluster()
{
   return *m_cluster;
}

// TODO schi add
shader_core_ctx* gpgpu_sim::get_shader(int id)
{
//    int clusters = m_config.m_shader_config.n_simt_clusters;
    int shaders_per_cluster = m_config.m_shader_config.n_simt_cores_per_cluster;
    int cluster = id/shaders_per_cluster;
    int shader_in_cluster = id%shaders_per_cluster;
    assert(shader_in_cluster < shaders_per_cluster);
    assert(cluster < m_config.m_shader_config.n_simt_clusters);

    return m_cluster[cluster]->get_core(shader_in_cluster);
}
#endif
