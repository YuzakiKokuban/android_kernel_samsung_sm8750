/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: ISC
 */

/**
 * DOC: Implement various notification handlers which are accessed
 * internally in mgmt_rx_srng component only.
 */

#include <wlan_mgmt_rx_srng_main.h>
#include "hal_api.h"
#include "hal_rx.h"
#include <init_deinit_lmac.h>
#include "target_if_mgmt_rx_srng.h"
#include "dp_internal.h"
#include "wmi_unified_api.h"

bool wlan_mgmt_rx_srng_cfg_enable(struct wlan_objmgr_psoc *psoc)
{
	struct mgmt_rx_srng_psoc_priv *psoc_priv;

	psoc_priv = wlan_objmgr_psoc_get_comp_private_obj(
					psoc, WLAN_UMAC_COMP_MGMT_RX_SRNG);
	if (!psoc_priv) {
		mgmt_rx_srng_err("psoc priv is NULL");
		return false;
	}

	return psoc_priv->mgmt_rx_srng_is_enable;
}

static QDF_STATUS
wlan_mgmt_rx_srng_alloc_buf(struct mgmt_rx_srng_pdev_priv *pdev_priv,
			    uint32_t id, qdf_dma_addr_t *paddr)
{
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	struct mgmt_rx_srng_desc *rx_desc;
	qdf_nbuf_t nbuf;
	int write_idx;

	nbuf = qdf_nbuf_alloc(pdev_priv->osdev, MGMT_RX_BUF_SIZE, 0, 4, false);
	if (!nbuf) {
		mgmt_rx_srng_err("Nbuf alloc failed");
		return QDF_STATUS_E_NOMEM;
	}

	status = qdf_nbuf_map_single(pdev_priv->osdev, nbuf,
				     QDF_DMA_FROM_DEVICE);
	if (QDF_IS_STATUS_ERROR(status)) {
		qdf_nbuf_free(nbuf);
		mgmt_rx_srng_err("Nbuf map failure for id %d", id);
		return QDF_STATUS_E_FAILURE;
	}
	*paddr = qdf_nbuf_get_frag_paddr(nbuf, 0);
	qdf_mem_dma_sync_single_for_device(pdev_priv->osdev, *paddr,
					   MGMT_RX_BUF_SIZE, DMA_FROM_DEVICE);

	write_idx = pdev_priv->write_idx;
	rx_desc = &pdev_priv->rx_desc[write_idx];

	rx_desc->nbuf = nbuf;
	rx_desc->pa = *paddr;
	rx_desc->cookie = id;
	rx_desc->in_use = true;

	return status;
}

void wlan_mgmt_rx_srng_reap_buf(struct wlan_objmgr_pdev *pdev,
				struct mgmt_srng_reap_event_params *param)
{
	struct mgmt_rx_srng_pdev_priv *pdev_priv;
	struct hal_buf_info buf_info;
	struct mgmt_rx_srng_hdr *hdr;
	struct wmi_unified *wmi_handle;
	void *srng;
	qdf_dma_addr_t paddr;
	QDF_STATUS status;
	uint16_t len;
	int num_buf_recv = 0;
	qdf_nbuf_t nbuf_head = NULL, nbuf_tail = NULL, nbuf, next;

	void *ring_entry;

	pdev_priv = wlan_objmgr_pdev_get_comp_private_obj(
					pdev, WLAN_UMAC_COMP_MGMT_RX_SRNG);
	if (!pdev_priv) {
		mgmt_rx_srng_err("psoc priv is NULL");
		return;
	}

	wmi_handle = lmac_get_pdev_wmi_handle(pdev);
	if (!wmi_handle) {
		mgmt_rx_srng_err("WMI handle is null in mgmt rx srng");
		return;
	}

	srng = pdev_priv->mgmt_rx_srng_cfg.srng;
	num_buf_recv = hal_srng_src_num_avail(pdev_priv->hal_soc, srng, true);

	hal_srng_access_start(pdev_priv->hal_soc, srng);
	while (num_buf_recv-- > 0) {
		qdf_mem_zero(&buf_info, sizeof(buf_info));
		ring_entry = hal_srng_src_get_next(pdev_priv->hal_soc, srng);
		if (!ring_entry) {
			mgmt_rx_srng_err("MGMT RX sring entry NULL");
			continue;
		}

		hal_rx_buf_cookie_rbm_get(pdev_priv->hal_soc,
					  (uint32_t *)ring_entry, &buf_info);
		status = wlan_mgmt_rx_srng_alloc_buf(pdev_priv,
						     buf_info.sw_cookie,
						     &paddr);

		if (QDF_IS_STATUS_SUCCESS(status)) {
			hal_rxdma_buff_addr_info_set(
				pdev_priv->hal_soc, ring_entry, paddr,
				pdev_priv->rx_desc[pdev_priv->write_idx].cookie,
				0);

			pdev_priv->write_idx =
			(pdev_priv->write_idx + 1) % MGMT_RX_SRNG_ENTRIES;
		} else {
			mgmt_rx_srng_err("Low mem: MGMT_SRNG buf alloc fail");
			break;
		}

		if (pdev_priv->rx_desc[pdev_priv->read_idx].in_use) {
			nbuf = pdev_priv->rx_desc[pdev_priv->read_idx].nbuf;
			pdev_priv->rx_desc[pdev_priv->read_idx].in_use = false;
			qdf_nbuf_unmap_single(pdev_priv->osdev, nbuf,
					      QDF_DMA_FROM_DEVICE);
			pdev_priv->read_idx =
			(pdev_priv->read_idx + 1) % MGMT_RX_SRNG_ENTRIES;
		} else {
			mgmt_rx_srng_err("Received invalid cookie buffer");
			qdf_assert_always(0);
		}

		if (!nbuf_head) {
			nbuf_head = nbuf;
			QDF_NBUF_CB_RX_NUM_ELEMENTS_IN_LIST(nbuf_head) = 1;
		} else {
			qdf_nbuf_set_next(nbuf_tail, nbuf);
			QDF_NBUF_CB_RX_NUM_ELEMENTS_IN_LIST(nbuf_head)++;
		}
		nbuf_tail = nbuf;
		qdf_nbuf_set_next(nbuf_tail, NULL);
	}
	hal_srng_access_end(pdev_priv->hal_soc, srng);

	//Processing of reaped buffers by sending them to WMI layer
	nbuf = nbuf_head;

	while (nbuf) {
		next = nbuf->next;
		hdr = (struct mgmt_rx_srng_hdr *)qdf_nbuf_data(nbuf);
		len = hdr->buff_len;

		/* FW adds the header to the buffer copied in the mgmt SRNG.
		 * This header provides host with info about the length of the
		 * buffer, use same to set the pkt len and remove the header.
		 */
		qdf_nbuf_set_pktlen(nbuf, len);
		if (qdf_nbuf_pull_head(nbuf, sizeof(*hdr)) == NULL) {
			mgmt_rx_srng_err("Pkt len insuffficient");
			qdf_nbuf_free(nbuf);
			nbuf = next;
			continue;
		}

		wmi_rx_buf_srng(wmi_handle, nbuf);
		nbuf = next;
	}
}

static
void wlan_mgmt_rx_srng_free_buffers(struct mgmt_rx_srng_pdev_priv *pdev_priv)
{
	int i;

	for (i = 0; i < MGMT_RX_SRNG_ENTRIES; i++) {
		if (pdev_priv->rx_desc[i].in_use) {
			qdf_nbuf_unmap_single(
				pdev_priv->osdev, pdev_priv->rx_desc[i].nbuf,
				QDF_DMA_FROM_DEVICE);
			qdf_nbuf_free(pdev_priv->rx_desc[i].nbuf);
		}
	}
}

static QDF_STATUS
wlan_mgmt_rx_srng_attach_buffers(struct mgmt_rx_srng_pdev_priv *pdev_priv)
{
	void *srng = pdev_priv->mgmt_rx_srng_cfg.srng;
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	qdf_dma_addr_t paddr;
	int i;
	void *ring_entry;

	hal_srng_access_start(pdev_priv->hal_soc, srng);
	for (i = 0; i < MGMT_RX_SRNG_ENTRIES - 1; i++) {
		status = wlan_mgmt_rx_srng_alloc_buf(pdev_priv, i, &paddr);
		if (QDF_IS_STATUS_ERROR(status)) {
			hal_srng_access_end(pdev_priv->hal_soc, srng);
			wlan_mgmt_rx_srng_free_buffers(pdev_priv);
			return status;
		}

		ring_entry = hal_srng_src_get_next(pdev_priv->hal_soc, srng);
		if (!ring_entry) {
			mgmt_rx_srng_err("Failure to get mgmt refill entry");
			QDF_BUG(0);
		}

		hal_rxdma_buff_addr_info_set(
			pdev_priv->hal_soc, ring_entry, paddr,
			pdev_priv->rx_desc[pdev_priv->write_idx].cookie, 0);
		pdev_priv->write_idx =
			(pdev_priv->write_idx + 1) % MGMT_RX_SRNG_ENTRIES;
	}
	hal_srng_access_end(pdev_priv->hal_soc, srng);

	return status;
}

static QDF_STATUS
wlan_mgmt_rx_srng_htt_setup_send(struct wlan_objmgr_pdev *pdev)
{
	struct cdp_soc_t *soc = cds_get_context(QDF_MODULE_ID_SOC);
	struct mgmt_rx_srng_pdev_priv *pdev_priv;
	struct mgmt_srng_cfg *mgmt_ring_cfg;
	QDF_STATUS status;

	pdev_priv = wlan_objmgr_pdev_get_comp_private_obj(
					pdev, WLAN_UMAC_COMP_MGMT_RX_SRNG);
	if (!pdev_priv) {
		mgmt_rx_srng_err("pdev priv is NULL");
		return QDF_STATUS_E_INVAL;
	}
	mgmt_ring_cfg = &pdev_priv->mgmt_rx_srng_cfg;
	status = dp_send_htt_mgmt_rx_buf_refil_srng_setup(soc,
							  mgmt_ring_cfg->srng);
	if (QDF_IS_STATUS_ERROR(status))
		mgmt_rx_srng_err("mgmt srng htt setup failed");

	return status;
}

static QDF_STATUS
wlan_mgmt_rx_srng_setup(struct mgmt_rx_srng_pdev_priv *pdev_priv)
{
	uint32_t entry_size = hal_srng_get_entrysize(pdev_priv->hal_soc,
						     RXDMA_BUF);
	struct wlan_objmgr_psoc *psoc;
	uint32_t ring_alloc_size;
	struct hal_srng_params ring_params = {0};
	struct mgmt_srng_cfg *mgmt_ring_cfg = &pdev_priv->mgmt_rx_srng_cfg;
	void *srng, *mem;
	QDF_STATUS status = QDF_STATUS_SUCCESS;

	if (!pdev_priv->osdev) {
		psoc = wlan_pdev_get_psoc(pdev_priv->pdev);
		if (!psoc) {
			mgmt_rx_srng_err("psoc obj is NULL");
			return QDF_STATUS_E_FAILURE;
		}
		pdev_priv->osdev = wlan_psoc_get_qdf_dev(psoc);
		if (!pdev_priv->osdev) {
			mgmt_rx_srng_err("no osdev");
			return QDF_STATUS_E_FAILURE;
		}
	}

	pdev_priv->rx_desc = (struct mgmt_rx_srng_desc *)qdf_mem_malloc(
		sizeof(struct mgmt_rx_srng_desc) * MGMT_RX_SRNG_ENTRIES);
	if (!pdev_priv->rx_desc) {
		mgmt_rx_srng_err("Failure to alloc rx desc for mgmt rx srng");
		return QDF_STATUS_E_NOMEM;
	}

	ring_alloc_size = (MGMT_RX_SRNG_ENTRIES * entry_size);

	mem = qdf_aligned_mem_alloc_consistent(
					pdev_priv->osdev, &ring_alloc_size,
					&mgmt_ring_cfg->base_vaddr_unaligned,
					&mgmt_ring_cfg->base_paddr_unaligned,
					&mgmt_ring_cfg->base_paddr_aligned, 32);

	if (!mem) {
		mgmt_rx_srng_err("Failed to alloc mem for mgmt rx srng");
		qdf_mem_free(pdev_priv->rx_desc);
		return QDF_STATUS_E_NOMEM;
	}

	qdf_mem_set(mgmt_ring_cfg->base_vaddr_unaligned, 0, ring_alloc_size);

	mgmt_ring_cfg->ring_alloc_size = ring_alloc_size;
	mgmt_ring_cfg->base_vaddr_aligned = mem;
	ring_params.ring_base_vaddr = mgmt_ring_cfg->base_vaddr_aligned;
	ring_params.ring_base_paddr =
		(qdf_dma_addr_t)mgmt_ring_cfg->base_paddr_aligned;
	ring_params.num_entries = MGMT_RX_SRNG_ENTRIES;

	srng = hal_srng_setup(pdev_priv->hal_soc, RXDMA_BUF,
			      MGMT_RX_BUF_REFILL_RING_IDX,
			      0, &ring_params, 0);
	if (!srng) {
		mgmt_rx_srng_err("mgmt rx srng setup failed");
		qdf_mem_free_consistent(pdev_priv->osdev,
					pdev_priv->osdev->dev,
					ring_alloc_size,
					mgmt_ring_cfg->base_vaddr_unaligned,
			(qdf_dma_addr_t)mgmt_ring_cfg->base_paddr_unaligned, 0);
		qdf_mem_free(pdev_priv->rx_desc);
		return QDF_STATUS_E_FAILURE;
	}
	mgmt_ring_cfg->srng = srng;

	pdev_priv->read_idx = 0;
	pdev_priv->write_idx = 0;

	return status;
}

static void wlan_mgmt_rx_srng_free(struct mgmt_rx_srng_pdev_priv *pdev_priv)
{
	struct mgmt_srng_cfg *mgmt_ring_cfg;

	mgmt_ring_cfg = &pdev_priv->mgmt_rx_srng_cfg;

	qdf_mem_free_consistent(pdev_priv->osdev,
				pdev_priv->osdev->dev,
				mgmt_ring_cfg->ring_alloc_size,
				mgmt_ring_cfg->base_vaddr_unaligned,
			(qdf_dma_addr_t)mgmt_ring_cfg->base_paddr_unaligned, 0);

	qdf_mem_free(pdev_priv->rx_desc);
}

QDF_STATUS wlan_mgmt_rx_srng_pdev_create_notification(
				struct wlan_objmgr_pdev *pdev, void *arg)
{
	struct hif_opaque_softc *hif_ctx = cds_get_context(QDF_MODULE_ID_HIF);
	struct mgmt_rx_srng_pdev_priv *pdev_priv;
	struct mgmt_rx_srng_psoc_priv *psoc_priv;
	struct wlan_objmgr_psoc *psoc;
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	int thres;

	if (!hif_ctx) {
		mgmt_rx_srng_err("invalid hif context");
		return QDF_STATUS_E_FAILURE;
	}

	psoc = wlan_pdev_get_psoc(pdev);
	if (!psoc) {
		mgmt_rx_srng_err("psoc is NULL");
		return QDF_STATUS_E_FAILURE;
	}

	psoc_priv = wlan_objmgr_psoc_get_comp_private_obj(
					psoc, WLAN_UMAC_COMP_MGMT_RX_SRNG);
	if (!psoc_priv) {
		mgmt_rx_srng_err("psoc priv is NULL");
		return QDF_STATUS_E_FAILURE;
	}

	/* Setup ring only if feature enabled in both host and target */
	if (!psoc_priv->mgmt_rx_srng_is_enable)
		return status;

	pdev_priv = qdf_mem_malloc(sizeof(*pdev_priv));
	if (!pdev_priv) {
		mgmt_rx_srng_err("Failure to alloc mgmt rx srng pdev priv");
		return QDF_STATUS_E_NOMEM;
	}

	status = wlan_objmgr_pdev_component_obj_attach(
					pdev, WLAN_UMAC_COMP_MGMT_RX_SRNG,
					pdev_priv, QDF_STATUS_SUCCESS);
	if (QDF_IS_STATUS_ERROR(status)) {
		mgmt_rx_srng_err("Failed to attach mgmt rx srng priv to pdev");
		qdf_mem_free(pdev_priv);
		psoc_priv->mgmt_rx_srng_is_enable = false;
		return status;
	}
	pdev_priv->pdev = pdev;
	thres = cfg_get(psoc, CFG_MGMT_RX_SRNG_REAP_EVENT_THRESHOLD);
	pdev_priv->hal_soc = hif_get_hal_handle(hif_ctx);

	status = wlan_mgmt_rx_srng_setup(pdev_priv);
	if (QDF_IS_STATUS_ERROR(status))
		goto free_pdev_priv;

	status = wlan_mgmt_rx_srng_attach_buffers(pdev_priv);
	if (QDF_IS_STATUS_ERROR(status))
		goto free_mgmt_rx_srng;

	status = tgt_mgmt_rx_srng_register_ev_handler(psoc);
	if (QDF_IS_STATUS_ERROR(status))
		goto free_srng_buffers;

	tgt_mgmt_rx_srng_send_reap_threshold(psoc, thres);

	status = wlan_mgmt_rx_srng_htt_setup_send(pdev);
	if (QDF_IS_STATUS_ERROR(status))
		goto unregister_ev_handlers;

	return status;

unregister_ev_handlers:
	tgt_mgmt_rx_srng_unregister_ev_handler(psoc);
free_srng_buffers:
	wlan_mgmt_rx_srng_free_buffers(pdev_priv);
free_mgmt_rx_srng:
	wlan_mgmt_rx_srng_free(pdev_priv);
free_pdev_priv:
	status = wlan_objmgr_pdev_component_obj_detach(
					pdev, WLAN_UMAC_COMP_MGMT_RX_SRNG,
					pdev_priv);
	if (QDF_IS_STATUS_ERROR(status))
		mgmt_rx_srng_err("Failed to detach mgmt rx srng pdev_priv");

	qdf_mem_free(pdev_priv);
	psoc_priv->mgmt_rx_srng_is_enable = false;
	return QDF_STATUS_E_FAILURE;
}

QDF_STATUS wlan_mgmt_rx_srng_pdev_destroy_notification(
				struct wlan_objmgr_pdev *pdev, void *arg)
{
	struct mgmt_rx_srng_pdev_priv *pdev_priv;
	struct mgmt_rx_srng_psoc_priv *psoc_priv;
	struct wlan_objmgr_psoc *psoc;
	QDF_STATUS status;

	psoc = wlan_pdev_get_psoc(pdev);
	if (!psoc) {
		mgmt_rx_srng_err("psoc is NULL");
		return QDF_STATUS_E_FAILURE;
	}

	psoc_priv = wlan_objmgr_psoc_get_comp_private_obj(
					psoc, WLAN_UMAC_COMP_MGMT_RX_SRNG);
	if (!psoc_priv) {
		mgmt_rx_srng_err("psoc priv is NULL");
		return QDF_STATUS_E_FAILURE;
	}

	if (!psoc_priv->mgmt_rx_srng_is_enable)
		return QDF_STATUS_SUCCESS;

	pdev_priv = wlan_objmgr_pdev_get_comp_private_obj(
					pdev, WLAN_UMAC_COMP_MGMT_RX_SRNG);
	if (!pdev_priv) {
		mgmt_rx_srng_err("pdev priv is NULL");
		return QDF_STATUS_E_FAILURE;
	}

	tgt_mgmt_rx_srng_unregister_ev_handler(psoc);
	wlan_mgmt_rx_srng_free_buffers(pdev_priv);
	wlan_mgmt_rx_srng_free(pdev_priv);

	status = wlan_objmgr_pdev_component_obj_detach(
					pdev, WLAN_UMAC_COMP_MGMT_RX_SRNG,
					pdev_priv);
	if (QDF_IS_STATUS_ERROR(status)) {
		mgmt_rx_srng_err("Failed to detach mgmt rx srng pdev_priv");
		return status;
	}
	qdf_mem_free(pdev_priv);

	return status;
}

QDF_STATUS wlan_mgmt_rx_srng_psoc_create_notification(
				struct wlan_objmgr_psoc *psoc, void *arg)
{
	struct mgmt_rx_srng_psoc_priv *psoc_priv;
	QDF_STATUS status;

	psoc_priv = qdf_mem_malloc(sizeof(*psoc_priv));
	if (!psoc_priv)
		return QDF_STATUS_E_NOMEM;

	status = wlan_objmgr_psoc_component_obj_attach(
				psoc, WLAN_UMAC_COMP_MGMT_RX_SRNG,
				psoc_priv, QDF_STATUS_SUCCESS);
	if (QDF_IS_STATUS_ERROR(status)) {
		mgmt_rx_srng_err("Failed to attach psoc component obj");
		goto free_psoc_priv;
	}
	psoc_priv->psoc = psoc;
	psoc_priv->mgmt_rx_srng_is_enable =
				cfg_get(psoc, CFG_FEATURE_MGMT_RX_SRNG_ENABLE);
	target_if_mgmt_rx_srng_register_rx_ops(&psoc_priv->rx_ops);
	target_if_mgmt_rx_srng_register_tx_ops(&psoc_priv->tx_ops);

	return status;

free_psoc_priv:
	qdf_mem_free(psoc_priv);
	return status;
}

QDF_STATUS wlan_mgmt_rx_srng_psoc_destroy_notification(
				struct wlan_objmgr_psoc *psoc, void *arg)
{
	struct mgmt_rx_srng_psoc_priv *psoc_priv;
	QDF_STATUS status;

	psoc_priv = wlan_objmgr_psoc_get_comp_private_obj(
					psoc, WLAN_UMAC_COMP_MGMT_RX_SRNG);
	if (!psoc_priv) {
		mgmt_rx_srng_err("psoc priv is NULL");
		return QDF_STATUS_E_FAILURE;
	}
	status = wlan_objmgr_psoc_component_obj_detach(
					psoc, WLAN_UMAC_COMP_MGMT_RX_SRNG,
					psoc_priv);
	if (QDF_IS_STATUS_ERROR(status)) {
		mgmt_rx_srng_err("Failed to detach psoc component obj");
		return status;
	}

	qdf_mem_free(psoc_priv);
	return status;
}
