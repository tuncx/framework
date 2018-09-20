/*---------------------------------------------------------------------------*/
/* Array2.h                                                    (C) 2000-2018 */
/*                                                                           */
/* Tableau 2D classique.                                                     */
/*---------------------------------------------------------------------------*/
#ifndef ARCCORE_UTILS_ARRAY2_H
#define ARCCORE_UTILS_ARRAY2_H
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#include "arccore/base/NotImplementedException.h"
#include "arccore/base/NotSupportedException.h"
#include "arccore/base/Array2View.h"

#include "arccore/collections/Array.h"

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

namespace Arccore
{

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*!
 * \ingroup Collection
 * \brief Classe repr�sentant un tableau 2D classique.
 *
 * Les instances de cette classe ne sont ni copiables ni affectable. Pour cr�er un
 * tableau copiable, il faut utiliser SharedArray2 (pour une s�mantique par
 * r�f�rence) ou UniqueArray2 (pour une s�mantique par valeur comme la STL).
 */
template<typename DataType>
class Array2
: private AbstractArray<DataType>
{
 protected:
  enum CloneBehaviour
  {
    CB_Clone,
    CB_Shared
  };
  enum InitBehaviour
  {
    IB_InitWithDefault,
    IB_NoInit
  };
  //TODO: verifier qu'on n'affecte pas m_p->dim1_size ou
  // m_p->dim2_size si m_p est TrueImpl::shared_null.
 private:
  typedef AbstractArray<DataType> Base;
  typedef typename Base::ConstReferenceType ConstReferenceType;
 protected:
  using AbstractArray<DataType>::m_p;
  using AbstractArray<DataType>::_setMP;
  using AbstractArray<DataType>::_destroy;
  using AbstractArray<DataType>::_internalDeallocate;
#ifndef ARCCORE_AVOID_DEPRECATED_ARRAY_CONSTRUCTOR
 public:
#else
 protected:
#endif
  Array2() : AbstractArray<DataType>() {}
  //! Cr�� un tableau de \a size1 * \a size2 �l�ments.
  Array2(Integer size1,Integer size2)
  : AbstractArray<DataType>() 
  {
    resize(size1,size2);
  }
  Array2(ConstArray2View<DataType> rhs) : AbstractArray<DataType>()
  {
    this->copy(rhs);
  }
 public:
  //! Cr�� un tableau vide avec un allocateur sp�cifique \a allocator
  explicit Array2(IMemoryAllocator* allocator)
  : AbstractArray<DataType>(allocator,0) {}
  /*!
   * \brief Cr�� un tableau de \a size1 * \a size2 �l�ments avec
   * un allocateur sp�cifique \a allocator.
   */
  Array2(IMemoryAllocator* allocator,Integer size1,Integer size2)
  : AbstractArray<DataType>(allocator,size1*size2)
  {
    resize(size1,size2);
  }
  ~Array2()
  {
  }
 private:
  //! Interdit
  void operator=(const Array2<DataType>& rhs);
  //! Interdit
  Array2(const Array2<DataType>& rhs); 

 protected:

  //! Constructeur par d�placement. Uniquement valide pour UniqueArray2.
  Array2(Array2<DataType>&& rhs) : AbstractArray<DataType>(std::move(rhs)) {}

 public:
  ArrayView<DataType> operator[](Integer i)
    {
      ARCCORE_CHECK_AT(i,m_p->dim1_size);
      return ArrayView<DataType>(ARCCORE_CAST_SMALL_SIZE(m_p->dim2_size),m_p->ptr + (m_p->dim2_size*i));
    }
  ConstArrayView<DataType> operator[](Integer i) const
    {
      ARCCORE_CHECK_AT(i,m_p->dim1_size);
      return ConstArrayView<DataType>(ARCCORE_CAST_SMALL_SIZE(m_p->dim2_size),m_p->ptr + (m_p->dim2_size*i));
    }
  DataType item(Integer i,Integer j)
  {
    ARCCORE_CHECK_AT(i,m_p->dim1_size);
    ARCCORE_CHECK_AT(j,m_p->dim2_size);
    return m_p->ptr[ (m_p->dim2_size*i) + j ];
  }
  void setItem(Integer i,Integer j,ConstReferenceType v)
  {
    ARCCORE_CHECK_AT(i,m_p->dim1_size);
    ARCCORE_CHECK_AT(j,m_p->dim2_size);
    m_p->ptr[ (m_p->dim2_size*i) + j ] = v;
  }
  //! El�ment d'indice \a i. V�rifie toujours les d�bordements
  ConstArrayView<DataType> at(Integer i) const
  {
    arccoreCheckAt(i,m_p->dim1_size);
    return this->operator[](i);
  }
  //! El�ment d'indice \a i. V�rifie toujours les d�bordements
  ArrayView<DataType> at(Integer i)
  {
    arccoreCheckAt(i,m_p->dim1_size);
    return this->operator[](i);
  }
  DataType at(Integer i,Integer j)
  {
    arccoreCheckAt(i,m_p->dim1_size);
    arccoreCheckAt(j,m_p->dim1_size);
    return m_p->ptr[ (m_p->dim2_size*i) + j ];
  }
  void fill(ConstReferenceType v)
  {
    this->_fill(v);
  }
  void clear()
  {
    this->resize(0,0);
  }
  Array2<DataType> clone()
  {
    return Array2<DataType>(this->constView());
  }
  void copy(ConstArray2View<DataType> rhs)
  {
    Integer total = rhs.totalNbElement();
    if (total==0){
      // Si la taille est nulle, il faut tout de meme faire une allocation
      // pour stocker les valeurs dim1_size et dim2_size (sinon, elle seraient
      // dans TrueImpl::shared_null)
      this->_reserve(4);
    }
    ConstArrayView<DataType> aview(total,rhs.unguardedBasePointer());
    Base::_copyView(aview);
    m_p->dim1_size = rhs.dim1Size();
    m_p->dim2_size = rhs.dim2Size();
    _arccoreCheckSharedNull();
  }
  //! Capacit� (nombre d'�l�ments allou�s) du tableau
  Integer capacity() const { return Base::capacity(); }

  //! R�serve de la m�moire pour \a new_capacity �l�ments
  void reserve(Integer new_capacity) { Base::_reserve(new_capacity); }

  //! Vue du tableau sous forme de tableau 1D
  ArrayView<DataType> viewAsArray()
  {
    return ArrayView<DataType>(ARCCORE_CAST_SMALL_SIZE(m_p->size),m_p->ptr);
  }
  //! Vue du tableau sous forme de tableau 1D
  ConstArrayView<DataType> viewAsArray() const
  {
    return ConstArrayView<DataType>(ARCCORE_CAST_SMALL_SIZE(m_p->size),m_p->ptr);
  }
 public:
  operator Array2View<DataType>()
  {
    return view();
  }
  operator ConstArray2View<DataType>() const
  {
    return constView();
  }
  Array2View<DataType> view()
  {
    return Array2View<DataType>(m_p->ptr,ARCCORE_CAST_SMALL_SIZE(m_p->dim1_size),ARCCORE_CAST_SMALL_SIZE(m_p->dim2_size));
  }
  ConstArray2View<DataType> constView() const
  {
    return ConstArray2View<DataType>(m_p->ptr,ARCCORE_CAST_SMALL_SIZE(m_p->dim1_size),ARCCORE_CAST_SMALL_SIZE(m_p->dim2_size));
  }
 public:
  Integer dim2Size() const { return ARCCORE_CAST_SMALL_SIZE(m_p->dim2_size); }
  Integer dim1Size() const { return ARCCORE_CAST_SMALL_SIZE(m_p->dim1_size); }
  void add(const DataType& value)
    {
      Base::_addRange(value,m_p->dim2_size);
      ++m_p->dim1_size;
      _arccoreCheckSharedNull();
    }

  //! Redimensionne uniquement la premi�re dimension en laissant la deuxi�me � l'identique
  void resize(Integer new_size)
  {
    _resize(new_size,IB_InitWithDefault);
  }
  //! Redimensionne uniquement la premi�re dimension en laissant la deuxi�me � l'identique
  void resizeNoInit(Integer new_size)
  {
    _resize(new_size,IB_NoInit);
  }
  //! R�alloue les deux dimensions
  void resize(Integer new_size1,Integer new_size2)
  {
    _resize(new_size1,new_size2,IB_InitWithDefault);
  }
  //! R�alloue les deux dimensions
  void resizeNoInit(Integer new_size1,Integer new_size2)
  {
    _resize(new_size1,new_size2,IB_NoInit);
  }
  Integer totalNbElement() const { return ARCCORE_CAST_SMALL_SIZE(m_p->dim1_size*m_p->dim2_size); }
 protected:
  //! Redimensionne uniquement la premi�re dimension en laissant la deuxi�me � l'identique
  void _resize(Int64 new_size,InitBehaviour rb)
  {
    Int64 old_size = m_p->dim1_size;
    if (new_size==old_size)
      return;
    _resize2(new_size,m_p->dim2_size,rb);
    m_p->dim1_size = new_size;
    _arccoreCheckSharedNull();
  }
  //! R�alloue les deux dimensions
  void _resize(Int64 new_size1,Int64 new_size2,InitBehaviour rb)
    {
      if (new_size2==m_p->dim2_size){
        _resize(new_size1,rb);
      }
      else if (totalNbElement()==0){
        _resizeFromEmpty(new_size1,new_size2,rb);
      }
      else if (new_size2<m_p->dim2_size){
        _resizeSameDim1ReduceDim2(new_size2,rb);
        _resize(new_size1,rb);
      }
      else if (new_size2>m_p->dim2_size){
        _resizeSameDim1IncreaseDim2(new_size2,rb);
        _resize(new_size1,rb);
      }
      else
        throw NotImplementedException("Array2::resize","already sized");
    }
  void _resizeFromEmpty(Int64 new_size1,Int64 new_size2,InitBehaviour rb)
    {
      _resize2(new_size1,new_size2,rb);
      m_p->dim1_size = new_size1;
      m_p->dim2_size = new_size2;
      _arccoreCheckSharedNull();
    }
  void _resizeSameDim1ReduceDim2(Int64 new_size2,InitBehaviour rb)
    {
      ARCCORE_ASSERT((new_size2<m_p->dim2_size),("Bad Size"));
      Int64 n = m_p->dim1_size;
      Int64 n2 = m_p->dim2_size;
      for( Int64 i=0; i<n; ++i ){
        for( Int64 j=0; j<new_size2; ++j )
          m_p->ptr[(i*new_size2)+j] = m_p->ptr[(i*n2)+j];
      }
      _resize2(n,new_size2,rb);
      m_p->dim2_size = new_size2;
      _arccoreCheckSharedNull();
    }
  void _resizeSameDim1IncreaseDim2(Int64 new_size2,InitBehaviour rb)
    {
      ARCCORE_ASSERT((new_size2>m_p->dim2_size),("Bad Size"));
      Int64 n = m_p->dim1_size;
      Int64 n2 = m_p->dim2_size;
      _resize2(n,new_size2,rb);
      // Recopie en partant de la fin pour �viter tout �crasement
      for( Int64 i=(n-1); i>=0; --i ){
        for( Int64 j=(n2-1); j>=0; --j )
          m_p->ptr[(i*new_size2)+j] = m_p->ptr[(i*n2)+j];
      }
      m_p->dim2_size = new_size2;
      _arccoreCheckSharedNull();
    }
  void _resize2(Int64 d1,Int64 d2,InitBehaviour rb)
  {
    Int64 new_size = d1*d2;
    // Si la nouvelle taille est nulle, il faut tout de meme faire une allocation
    // pour stocker les valeurs dim1_size et dim2_size (sinon, elle seraient
    // dans TrueImpl::shared_null
    if (new_size==0)
      this->_reserve(4);
    if (rb==IB_InitWithDefault)
      Base::_resize(new_size,DataType());
    else if (rb==IB_NoInit)
      Base::_resize(new_size);
    else
      throw NotSupportedException("Array2::_resize2","invalid value InitBehaviour");
  }
  void _move(Array2<DataType>& rhs)
  {
    Base::_move(rhs);
  }
  void _swap(Array2<DataType>& rhs)
  {
    Base::_swap(rhs);
  }
 private:
  void _arccoreCheckSharedNull()
  {
    if (m_p==ArrayImplBase::shared_null)
      ArrayImplBase::throwBadSharedNull();
  }
};


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*!
 * \ingroup Collection
 *
 * \brief Vecteur de donn�es 2D partag�es avec s�mantique par r�f�rence.
 *
 * \code
 * SharedArray2<int> a1(5,7);
 * SharedArray2<int> a2;
 * a2 = a1;
 * a1[3][6] = 1;
 * a2[1][2] = 2;
 * \endcode
 *
 * Dans l'exemple pr�c�dent, \a a1 et \a a2 font r�f�rence � la m�me zone
 * m�moire et donc a1[3][6] aura la m�me valeur que a2[1][2].
 *
 * Pour avoir un vecteur qui recopie les �l�ments lors de l'affectation,
 * il faut utiliser la classe UniqueArray2.
 *
 * Pour plus d'informations, se reporter � SharedArray.
 */
template<typename T>
class SharedArray2
: public Array2<T>
{
 protected:

  using Array2<T>::m_p;

 public:

  typedef SharedArray2<T> ThatClassType;
  typedef AbstractArray<T> BaseClassType;
  typedef typename BaseClassType::ConstReferenceType ConstReferenceType;

 public:

 public:
  //! Cr�� un tableau vide
  SharedArray2() : Array2<T>(), m_next(nullptr), m_prev(nullptr) {}
  //! Cr�� un tableau de \a size1 * \a size2 �l�ments.
  SharedArray2(Integer size1,Integer size2)
  : Array2<T>(), m_next(nullptr), m_prev(nullptr)
  {
    this->resize(size1,size2);
  }
  //! Cr�� un tableau en recopiant les valeurs de la value \a view.
  SharedArray2(const ConstArray2View<T>& view)
  : Array2<T>(view), m_next(nullptr), m_prev(nullptr)
  {
  }
  //! Cr�� un tableau faisant r�f�rence � \a rhs.
  SharedArray2(const SharedArray2<T>& rhs)
  : Array2<T>(), m_next(nullptr), m_prev(nullptr)
  {
    _initReference(rhs);
  }
  //! Cr�� un tableau en recopiant les valeurs \a rhs.
  inline SharedArray2(const UniqueArray2<T>& rhs);
  //! Change la r�f�rence de cette instance pour qu'elle soit celle de \a rhs.
  void operator=(const SharedArray2<T>& rhs)
  {
    this->_operatorEqual(rhs);
  }
  //! Copie les valeurs de \a rhs dans cette instance.
  inline void operator=(const UniqueArray2<T>& rhs);
  //! Copie les valeurs de la vue \a rhs dans cette instance.
  void operator=(const ConstArray2View<T>& rhs)
  {
    this->copy(rhs);
  }
  //! D�truit l'instance
  ~SharedArray2() override
  {
    _removeReference();
  }
 public:

  //! Clone le tableau
  SharedArray2<T> clone() const
  {
    return SharedArray2<T>(this->constView());
  }

 protected:
  void _initReference(const ThatClassType& rhs)
  {
    this->_setMP(rhs.m_p);
    _addReference(&rhs);
    ++m_p->nb_ref;
  }
  //! Mise � jour des r�f�rences
  void _updateReferences() override final
  {
    for( ThatClassType* i = m_prev; i; i = i->m_prev )
      i->_setMP(m_p);
    for( ThatClassType* i = m_next; i; i = i->m_next )
      i->_setMP(m_p);
  }
  //! Mise � jour des r�f�rences
  Integer _getNbRef() override final
  {
    Integer nb_ref = 1;
    for( ThatClassType* i = m_prev; i; i = i->m_prev )
      ++nb_ref;
    for( ThatClassType* i = m_next; i; i = i->m_next )
      ++nb_ref;
    return nb_ref;
  }
  /*!
   * \brief Ins�re cette instance dans la liste cha�n�e.
   * L'instance est ins�r�e � la position de \a new_ref.
   * \pre m_prev==0
   * \pre m_next==0;
   */
  void _addReference(const ThatClassType* new_ref)
  {
    ThatClassType* nf = const_cast<ThatClassType*>(new_ref);
    ThatClassType* prev = nf->m_prev;
    nf->m_prev = this;
    m_prev = prev;
    m_next = nf;
    if (prev)
      prev->m_next = this;
  }
  //! Supprime cette instance de la liste cha�n�e des r�f�rences
  void _removeReference()
  {
    if (m_prev)
      m_prev->m_next = m_next;
    if (m_next)
      m_next->m_prev = m_prev;
  }
  //! D�truit l'instance si plus personne ne la r�f�rence
  void _checkFreeMemory()
  {
    if (m_p->nb_ref==0){
      this->_destroy();
      this->_internalDeallocate();
    }
  }
  void _operatorEqual(const ThatClassType& rhs)
  {
    if (&rhs!=this){
      _removeReference();
      _addReference(&rhs);
      ++rhs.m_p->nb_ref;
      --m_p->nb_ref;
      _checkFreeMemory();
      this->_setMP(rhs.m_p);
    }
  }
 private:
  ThatClassType* m_next; //!< R�f�rence suivante dans la liste cha�n�e
  ThatClassType* m_prev; //!< R�f�rence pr�c�dente dans la liste cha�n�e
 private:
  //! Interdit
  void operator=(const Array2<T>& rhs);
};

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*!
 * \ingroup Collection
 *
 * \brief Vecteur de donn�es 2D avec s�mantique par valeur (style STL).
 *
 * Cette classe est le pendant de UniqueArray pour les tableaux 2D.
 */
template<typename T>
class UniqueArray2
: public Array2<T>
{
 public:

  typedef AbstractArray<T> BaseClassType;
  typedef typename BaseClassType::ConstReferenceType ConstReferenceType;

 public:

 public:
  //! Cr�� un tableau vide
  UniqueArray2() : Array2<T>() {}
  //! Cr�� un tableau de \a size1 * \a size2 �l�ments.
  explicit UniqueArray2(Integer size1,Integer size2) : Array2<T>()
  {
    this->resize(size1,size2);
  }
  //! Cr�� un tableau en recopiant les valeurs de la value \a view.
  UniqueArray2(const ConstArray2View<T>& view) : Array2<T>(view)
  {
  }
  //! Cr�� un tableau en recopiant les valeurs \a rhs.
  UniqueArray2(const Array2<T>& rhs) : Array2<T>(rhs.constView())
  {
  }
  //! Cr�� un tableau en recopiant les valeurs \a rhs.
  UniqueArray2(const UniqueArray2<T>& rhs) : Array2<T>(rhs.constView())
  {
  }
  //! Cr�� un tableau en recopiant les valeurs \a rhs.
  UniqueArray2(const SharedArray2<T>& rhs) : Array2<T>(rhs.constView())
  {
  }
  //! Cr�� un tableau vide avec un allocateur sp�cifique \a allocator
  explicit UniqueArray2(IMemoryAllocator* allocator)
  : Array2<T>(allocator) {}
  /*!
   * \brief Cr�� un tableau de \a size1 * \a size2 �l�ments avec
   * un allocateur sp�cifique \a allocator.
   */
  UniqueArray2(IMemoryAllocator* allocator,Integer size1,Integer size2)
  : Array2<T>(allocator,size1,size2) { }
  //! Constructeur par d�placement. \a rhs est invalid� apr�s cet appel
  UniqueArray2(UniqueArray2<T>&& rhs) ARCCORE_NOEXCEPT : Array2<T>(std::move(rhs)) {}
  //! Copie les valeurs de \a rhs dans cette instance.
  void operator=(const Array2<T>& rhs)
  {
    this->copy(rhs.constView());
  }
  //! Copie les valeurs de \a rhs dans cette instance.
  void operator=(const SharedArray2<T>& rhs)
  {
    this->copy(rhs.constView());
  }
  //! Copie les valeurs de \a rhs dans cette instance.
  void operator=(const UniqueArray2<T>& rhs)
  {
    this->copy(rhs.constView());
  }
  //! Copie les valeurs de la vue \a rhs dans cette instance.
  void operator=(ConstArray2View<T> rhs)
  {
    this->copy(rhs);
  }
  //! Op�rateur de recopie par d�placement. \a rhs est invalid� apr�s cet appel.
  void operator=(UniqueArray2<T>&& rhs) ARCCORE_NOEXCEPT
  {
    this->_move(rhs);
  }
  //! D�truit le tableau
  ~UniqueArray2() override
  {
  }
 public:
  /*!
   * \brief �change les valeurs de l'instance avec celles de \a rhs.
   *
   * L'�change se fait en temps constant et sans r�allocation.
   */
  void swap(UniqueArray2<T>& rhs) ARCCORE_NOEXCEPT
  {
    this->_swap(rhs);
  }
};

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*!
 * \brief �change les valeurs de \a v1 et \a v2.
 *
 * L'�change se fait en temps constant et sans r�allocation.
 */
template<typename T> inline void
swap(UniqueArray2<T>& v1,UniqueArray2<T>& v2)
{
  v1.swap(v2);
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

template<typename T> inline SharedArray2<T>::
SharedArray2(const UniqueArray2<T>& rhs)
: Array2<T>(rhs.constView())
, m_next(nullptr)
, m_prev(nullptr)
{
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

template<typename T> inline void SharedArray2<T>::
operator=(const UniqueArray2<T>& rhs)
{
  this->copy(rhs);
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

} // End namespace Arccore

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#endif  
