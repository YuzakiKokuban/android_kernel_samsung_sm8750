/*
** Copyright (c) 2019, The Linux Foundation. All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above
**     copyright notice, this list of conditions and the following
**     disclaimer in the documentation and/or other materials provided
**     with the distribution.
**   * Neither the name of The Linux Foundation nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
** WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
** ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
** BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
** CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
** SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
** BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
** WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
** OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
** IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**/

#ifndef __SND_CARD_DEF_H__
#define __SND_CARD_DEF_H__

enum snd_node_type {
    SND_NODE_TYPE_MIN = 0,
    SND_NODE_TYPE_PCM = SND_NODE_TYPE_MIN,
    SND_NODE_TYPE_MIXER,
    SND_NODE_TYPE_COMPR,
    SND_NODE_TYPE_MAX,
};

enum snd_dev_type {
    SND_NODE_DEV_TYPE_HW = 0,
    SND_NODE_DEV_TYPE_PLUGIN,
};

/*
 * TBD: Need to add compile time functionality to stub out APIs if
 * snd-card-def library is not compiled.
 */
#if defined(SOME_COMPILE_TIME_FLAG_HERE)
void *snd_card_def_get_card(unsigned int card)
{
		return NULL;
}

void snd_card_def_put_card(void *card_node)
{
}

void *snd_card_def_get_node(void *card_node, unsigned int id,
							int type)
{
	return NULL;
}

int snd_card_def_get_num_node(void *card_node, int type)
{
	return 0;
}

int snd_card_def_get_nodes_for_type(void *card_node, int type,
                                    void **list, int num_nodes)
{
	return -EINVAL;
}

int snd_card_def_get_int(void *node, const char *prop,
						 int *val)
{
	return -EINVAL;
}

int snd_card_def_get_str(void *node, const char *prop,
						 char **val)
{
	return -EINVAL;
}

#else

/*
 * snd_card_def_get_card:
 *	Get handle to the sound card definition entry in the
 *	sound card definition XML
 *
 * @card: card-id (either physical or virtual)
 *
 * Returns:
 *	- Valid pointer pointing to the card handle or
 *	- NULL in error cases or if valid entry cannot be
 *	  found in the card definition file.
 */
void *snd_card_def_get_card(unsigned int card);

/*
 * snd_card_def_put_card:
 *	Release resources associated with handle to the
 *      sound card definition entry in the sound card
 *      definition XML.
 *
 * @card_node: card handle
 */
void snd_card_def_put_card(void *card_node);

/*
 * snd_card_def_get_node:
 *	Get handle to the node from the sound card definition XML
 *
 * @card_node: Handle to the card node
 *			   returned from a call to snd_card_def_get_card
 * @id: Id for the node (PCM/Compress/mixer device)
 * @type: enum to indicate type of node to be found
 *
 * Returns:
 *	- Valid pointer pointing to the node requested
 *	- NULL in error case or entry not found
 */
void *snd_card_def_get_node(void *card_node, unsigned int id,
							int type);

/*
 * snd_card_def_get_num_node:
 *	Get number of nodes from the sound card definition XML
 *
 * @card_node: Handle to the card node
 *			   returned from a call to snd_card_def_get_card
 * @type: enum to indicate type of node to be found
 *
 * Returns:
 *	- number of nodes of the requested type
 *	- 0 in error case or entry not found
 */
int snd_card_def_get_num_node(void *card_node, int type);

/*
 * snd_card_def_get_nodes_for_type:
 *	Get list of nodes from the sound card definition XML
 *
 * @card_node: Handle to the card node
 *			   returned from a call to snd_card_def_get_card
 * @type: enum to indicate type of node to be found
 * @list: List of device nodes requested
 * @num_nodes: Count of nodes requested
 *
 * Returns:
 *	- 0 on success
 *	- non zero in error case or entry not found
 */
int snd_card_def_get_nodes_for_type(void *card_node, int type,
                                    void **list, int num_nodes);
/*
 * snd_card_def_get_int:
 *	Get the integer value for the property under a node in the
 *	sound card definition XML
 *
 * @node: Handle to node under which the property is to be found
 *		  returned from a call to snd_card_def_get_node
 * @prop: Name of the property to be read under the given node
 * @val: pointer to caller allocated memory where the integer
 *		 value can be stored
 *
 * Returns:
 *	- zero on success with the value copied in val
 *	- negative error code on failure
 */
int snd_card_def_get_int(void *node, const char *prop,
						 int *val);

/*
 * snd_card_def_get_str:
 *	Get the string value for the property under a node in the
 *	sound card definition XML
 *
 * @node: Handle to node under which the property is to be found
 *		  returned from a call to snd_card_def_get_node
 * @prop: Name of the property to be read under the given node
 * @val: pointer to caller allocated memory where the string
 *		 value can be stored
 *
 * Returns:
 *	- zero on success with the value copied in val
 *	- negative error code on failure
 */
int snd_card_def_get_str(void *node, const char *prop,
						 char **val);

#endif // end of SOME_COMPILE_TIME_FLAG_HERE
#endif // end of __SND_CARD_DEF_H__
