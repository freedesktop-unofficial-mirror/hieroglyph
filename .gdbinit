# .gdbinit
# Copyright (C) 2006,2010 Akira TAGOH

# Authors:
#   Akira TAGOH  <akira@tagoh.org>

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330,
# Boston, MA 02111-1307, USA.

## constant
set $_hg_q_bit_type0  = 0
set $_hg_q_bit_type1  = 1
set $_hg_q_bit_type2  = 2
set $_hg_q_bit_type3  = 3
set $_hg_q_bit_type4  = 4
set $_hg_q_bit_exec   = 5
set $_hg_q_bit_read   = 6
set $_hg_q_bit_write  = 7
set $_hg_q_bit_access = 8
set $_hg_q_bit_memid0 = 9
set $_hg_q_bit_memid1 = 10
set $_hg_v_bit_idx00 = 0
set $_hg_v_bit_idx01 = 1
set $_hg_v_bit_idx02 = 2
set $_hg_v_bit_idx03 = 3
set $_hg_v_bit_idx04 = 4
set $_hg_v_bit_idx05 = 5
set $_hg_v_bit_idx06 = 6
set $_hg_v_bit_idx07 = 7
set $_hg_v_bit_idx08 = 8
set $_hg_v_bit_idx09 = 9
set $_hg_v_bit_idx10 = 10
set $_hg_v_bit_idx11 = 11
set $_hg_v_bit_idx12 = 12
set $_hg_v_bit_idx13 = 13
set $_hg_v_bit_idx14 = 14
set $_hg_v_bit_idx15 = 15
set $_hg_v_bit_idx16 = 16
set $_hg_v_bit_idx17 = 17
set $_hg_v_bit_idx18 = 18
set $_hg_v_bit_idx19 = 19
set $_hg_v_bit_idx20 = 20
set $_hg_v_bit_idx21 = 21
set $_hg_v_bit_idx22 = 22
set $_hg_v_bit_idx23 = 23
set $_hg_v_bit_page0 = 24
set $_hg_v_bit_page1 = 25
set $_hg_v_bit_page2 = 26
set $_hg_v_bit_page3 = 27
set $_hg_v_bit_page4 = 28
set $_hg_v_bit_page5 = 29
set $_hg_v_bit_page6 = 30
set $_hg_v_bit_page7 = 31

define _sethgquarkinfo
  set $_memid = ($arg0 >> 32 & (((1 << ($_hg_q_bit_memid1-$_hg_q_bit_memid0+1)) - 1) << $_hg_q_bit_memid0)) >> $_hg_q_bit_memid0
  set $_read  = ($arg0 >> 32 & (((1 << ($_hg_q_bit_read-$_hg_q_bit_read+1)) - 1) << $_hg_q_bit_read)) >> $_hg_q_bit_read
  set $_write  = ($arg0 >> 32 & (((1 << ($_hg_q_bit_write-$_hg_q_bit_write+1)) - 1) << $_hg_q_bit_write)) >> $_hg_q_bit_write
  set $_exec  = ($arg0 >> 32 & (((1 << ($_hg_q_bit_exec-$_hg_q_bit_exec+1)) - 1) << $_hg_q_bit_exec)) >> $_hg_q_bit_exec
  set $_access = ($arg0 >> 32 & (((1 << ($_hg_q_bit_access-$_hg_q_bit_access+1)) - 1) << $_hg_q_bit_access)) >> $_hg_q_bit_access
  set $_typeid = ($arg0 >> 32 & (((1 << ($_hg_q_bit_type4-$_hg_q_bit_type0+1)) - 1) << $_hg_q_bit_type0)) >> $_hg_q_bit_type0
  set $_value = ($arg0 & ((1LL << 32) - 1))
  set $_page = ($_value & (((1 << ($_hg_v_bit_page7-$_hg_v_bit_page0+1)) - 1) << $_hg_v_bit_page0)) >> $_hg_v_bit_page0
  set $_idx = ($_value & (((1 << ($_hg_v_bit_idx23-$_hg_v_bit_idx00+1)) - 1) << $_hg_v_bit_idx00)) >> $_hg_v_bit_idx00
end

define _sethgallocatorinfo
  set $_block_size = 64
end

define hgquarkinfo
  _sethgquarkinfo $arg0

  if ($arg0 == 0)
    printf "nil\n"
  else
    printf "(hg_quark_t)%p\n", $arg0
    printf "  [mem ID]: %d\n", $_memid
    printf "  [access]: "
    if ($_read == 0)
      printf "-"
    else
      printf "r"
    end
    if ($_write == 0)
      printf "-"
    else
      printf "w"
    end
    if ($_exec == 0)
      printf "-"
    else
      printf "x"
    end
    if ($_access == 0)
      printf "-"
    else
      printf "a"
    end
    printf "\n"
    printf "  [type]: "
    if ($_typeid == 0)
      printf "null"
    else
      if ($_typeid == 1)
	printf "integer"
      else
	if ($_typeid == 2)
	  printf "real"
	else
	  if ($_typeid == 3)
	    printf "name"
	  else
	    if ($_typeid == 4)
	      printf "boolean"
	    else
	      if ($_typeid == 5)
		printf "string"
	      else
		if ($_typeid == 6)
		  printf "immediately evaluated name"
		else
		  if ($_typeid == 7)
		    printf "dict"
		  else
		    if ($_typeid == 8)
		      printf "oper"
		    else
		      if ($_typeid == 9)
			printf "array"
		      else
			if ($_typeid == 10)
			  printf "mark"
			else
			  if ($_typeid == 11)
			    printf "file"
			  else
			    if ($_typeid == 12)
			      printf "save"
			    else
			      if ($_typeid == 13)
				printf "stack"
			      else
				if ($_typeid == 14)
				  printf "dict node"
				else
				  if ($_typeid == 15)
				    printf "path"
				  else
				    if ($_typeid == 16)
				      printf "gstate"
				    else
				      printf "unknown"
				    end
				  end
				end
			      end
			    end
			  end
			end
		      end
		    end
		  end
		end
	      end
	    end
	  end
	end
      end
    end
    printf "\n"
    printf "  [value]: page %d, index %d (%p)\n", $_page, $_idx, $_value
  end
end

define hggetblock
  _sethgallocatorinfo
  set $_heaps = $arg0
  set $_page = $arg1
  set $_idx = $arg2
  set $_block = (hg_allocator_block_t *)((hg_quark_t)$_heaps[$_page] + (($_idx - 1) * $_block_size))
  printf "(hg_allocator_block_t *) %p\n", $_block
end

define hggetobj
  hggetblock $arg0 $arg1 $arg2
  set $_obj = (hg_pointer_t)((hg_char_t *)($_block) + ((sizeof (hg_allocator_block_t) + (sizeof (void *)) - 1) & ~((sizeof (void *)) - 1)))
  printf "(hg_pointer_t) %p\n", $_obj
end

define hgquarkgetblock
  _sethgquarkinfo $arg0
  call hg_mem_spool_get($_memid)
  set $_mem = $
  set $_heaps = ((hg_allocator_private_t *)$_mem->data)->heaps
  hggetblock  $_heaps $_page $_idx
end

define hgquarkgetobj
  _sethgquarkinfo $arg0
  call hg_mem_spool_get($_memid)
  set $_mem = $
  set $_heaps = ((hg_allocator_private_t *)$_mem->data)->heaps
  hggetobj $heaps $_page $_idx
end

define _hggetmemobj
  set $_obj = (HgMemObject *)($arg0)
  if ($_obj->magic != 0x48474d4f)
    set $_obj = (HgMemObject *)((gsize)($arg0) - sizeof (HgMemObject))
    if ($_obj->magic != 0x48474d4f)
      set $_obj = 0
    end
  end
end

define _hggethgobj
  _hggetmemobj $arg0
  set $_hobj = 0
  if ($_obj != 0)
    if (($_obj->flags & (1 << 15)) != 0)
      set $_hobj = (HgObject *)$_obj->data
    end
  end
end

define _hgmeminfo
  dont-repeat
  _hggetmemobj $arg0
  if ($_obj == 0)
    printf "Invalid object %p\n", $arg0
  else
    printf "(HgMemObject *)%p - [subid: %p] ", $_obj, $_obj->subid
    printf "[heap id: %d] ", ($_obj->flags >> 24) & 0xff
    printf "[pool: %p] ", $_obj->pool
    printf "[block_size: %u] ", $_obj->block_size
    printf "[mark age: %d] ", ($_obj->flags >> 16) & 0xff
    set $f = $_obj->flags & 0xffff
    set $o = 0
    printf "[flags: "
    if (($f & (1 << 0)) != 0)
      printf "RESTORABLE"
      set $o = 1
    end
    if (($f & (1 << 1)) != 0)
      if $o != 0
	printf "|"
      end
      printf "COMPLEX"
      set $o = 1
    end
    if (($f & (1 << 2)) != 0)
      if $o != 0
	printf "|"
      end
      printf "LOCK"
      set $o = 1
    end
    if (($f & (1 << 3)) != 0)
      if $o != 0
	printf "|"
      end
      printf "COPYING"
      set $o = 1
    end
    if (($f & (1 << 15)) != 0)
      if $o != 0
	printf "|"
      end
      printf "HGOBJECT"
      set $o = 1
    end
    printf "]\n"
  end
end

define _hgobjinfo
  dont-repeat
  _hggethgobj $arg0
  if ($_hobj == 0)
    printf "Invalid object %p\n", $arg0
  else
    printf "(HgObject *)%p - ", $_hobj
    set $f = $_hobj->state & 0xffff
    set $o = 0
    printf "[state: "
    if (($f & (1 << 0)) != 0)
      printf "READABLE"
      set $o = 1
    end
    if (($f & (1 << 1)) != 0)
      if $o != 0
	printf "|"
      end
      printf "WRITABLE"
      set $o = 1
    end
    if (($f & (1 << 2)) != 0)
      if $o != 0
	printf "|"
      end
      printf "EXECUTABLE"
      set $o = 1
    end
    printf "] "
    printf "[user data: %d] ", ($_hobj->state >> 16) & 0xff
    printf "[vtable id: %d] ", ($_hobj->state >> 24) & 0xff
    printf "\n"
  end
end

define hgnodeprint
  dont-repeat
  _hggethgobj $arg0
  if ($_hobj == 0)
    printf "%p isn't an valid object managed by hieroglyph.\n", $arg0
  else
    _hgmeminfo $arg0
    _hgobjinfo $arg0
    set $_node = (HgValueNode *)$_hobj
    set $_nodetype = (HgValueType)(($_hobj->state >> 16) & 0xff)
    printf "(HgValueNode *)%p - [type: ", $_node
    if ($_nodetype == HG_TYPE_VALUE_BOOLEAN)
      printf "BOOLEAN] [%d]\n", $_node->v.boolean
    end
    if ($_nodetype == HG_TYPE_VALUE_INTEGER)
      printf "INTEGER] [%d]\n", $_node->v.integer
    end
    if ($_nodetype == HG_TYPE_VALUE_REAL)
      printf "REAL] [%f]\n", $_node->v.real
    end
    if ($_nodetype == HG_TYPE_VALUE_NAME)
      printf "NAME] [%s]\n", (char *)$_node->v.pointer
    end
    if ($_nodetype == HG_TYPE_VALUE_ARRAY)
      printf "ARRAY]\n"
      print (HgArray *)$_node->v.pointer
    end
    if ($_nodetype == HG_TYPE_VALUE_STRING)
      printf "STRING]\n"
      print (HgString *)$_node->v.pointer
    end
    if ($_nodetype == HG_TYPE_VALUE_DICT)
      printf "DICT]\n"
      print (HgDict *)$_node->v.pointer
    end
    if ($_nodetype == HG_TYPE_VALUE_NULL)
      printf "NULL]\n"
    end
    if ($_nodetype == HG_TYPE_VALUE_POINTER)
      printf "OPERATOR]\n"
      print (LibrettoOperator *)$_node->v.pointer
    end
    if ($_nodetype == HG_TYPE_VALUE_MARK)
      printf "MARK]\n"
    end
    if ($_nodetype == HG_TYPE_VALUE_FILE)
      printf "FILE]\n"
      print (HgFileObject *)$_node->v.pointer
    end
    if ($_nodetype == HG_TYPE_VALUE_SNAPSHOT)
      printf "SNAPSHOT]\n"
      print (HgMemSnapshot *)$_node->v.pointer
    end
  end
end
document hgnodeprint
Print the value of the node given as argument.
end

define hgarrayprint
  dont-repeat
  _hggethgobj $arg0
  if ($_hobj == 0)
    printf "%p isn't a valid object managed by hieroglyph.\n", $arg0
  else
    _hgmeminfo $arg0
    _hgobjinfo $arg0
    set $_array = (HgArray *)$_hobj
    printf "(HgArray *)%p - [%d/%d]\n", $_array, $_array->n_arrays, $_array->allocated_arrays
    set $_i = 0
    while ($_i < $_array->n_arrays)
      set $_p = $_array->current[$_i]
      printf "[%d]: ", $_i
      hgnodeprint $_array->current[$_i]
      set $_i++
    end
  end
end

define hgdictprint
  dont-repeat
end

define hgstringprint
  dont-repeat
  _hggethgobj $arg0
  if ($_hobj == 0)
    printf "%p isn't a valid object managed by hieroglyph.\n", $arg0
  else
    _hgmeminfo $arg0
    _hgobjinfo $arg0
    set $_string = (HgString *)$_hobj
    printf "(HgString *)%p - ", $_string
    if ($_string->substring_offset != 0)
      printf "[substring] "
    end
    printf "[alloc: %d] [length: %d]\n", $_string->allocated_size, $_string->length
    output "  [string: "
    set $_i = 0
    while ($_i < $_string->length)
      printf "%c", $_string->current[$_i]
      set $_i++
    end
    printf "]\n"
  end
end

define hgoperprint
  dont-repeat
end

define hgfileprint
  dont-repeat
end

define hgsnapprint
  dont-repeat
end
